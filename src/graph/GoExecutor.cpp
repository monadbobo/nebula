/* Copyright (c) 2018 vesoft inc. All rights reserved.
 *
 * This source code is licensed under Apache 2.0 License,
 * attached with Common Clause Condition 1.0, found in the LICENSES directory.
 */

#include "base/Base.h"
#include "graph/GoExecutor.h"
#include "dataman/RowReader.h"
#include "dataman/RowSetReader.h"
#include "dataman/ResultSchemaProvider.h"

namespace nebula {
namespace graph {

GoExecutor::GoExecutor(Sentence *sentence, ExecutionContext *ectx) : TraverseExecutor(ectx) {
    // The RTTI is guaranteed by Sentence::Kind,
    // so we use `static_cast' instead of `dynamic_cast' for the sake of efficiency.
    sentence_ = static_cast<GoSentence*>(sentence);
}


Status GoExecutor::prepare() {
    DCHECK(sentence_ != nullptr);
    Status status;
    expCtx_ = std::make_unique<ExpressionContext>();
    do {
        status = checkIfGraphSpaceChosen();
        if (!status.ok()) {
            break;
        }
        status = prepareStep();
        if (!status.ok()) {
            break;
        }
        status = prepareFrom();
        if (!status.ok()) {
            break;
        }
        status = prepareOver();
        if (!status.ok()) {
            break;
        }
        status = prepareWhere();
        if (!status.ok()) {
            break;
        }
        status = prepareYield();
        if (!status.ok()) {
            break;
        }
        status = prepareNeededProps();
        if (!status.ok()) {
            break;
        }
        status = prepareDistinct();
        if (!status.ok()) {
            break;
        }
    } while (false);

    if (!status.ok()) {
        LOG(ERROR) << "Preparing failed: " << status;
        return status;
    }

    return status;
}


void GoExecutor::execute() {
    FLOG_INFO("Executing Go: %s", sentence_->toString().c_str());
    auto status = setupStarts();
    if (!status.ok()) {
        onError_(std::move(status));
        return;
    }
    if (starts_.empty()) {
        onEmptyInputs();
        return;
    }
    if (distinct_) {
        std::unordered_set<VertexID> uniqID;
        for (auto id : starts_) {
            uniqID.emplace(id);
        }
        starts_ = std::vector<VertexID>(uniqID.begin(), uniqID.end());
    }
    stepOut();
}


void GoExecutor::feedResult(std::unique_ptr<InterimResult> result) {
    inputs_ = std::move(result);
}


Status GoExecutor::prepareStep() {
    auto *clause = sentence_->stepClause();
    if (clause != nullptr) {
        steps_ = clause->steps();
        upto_ = clause->isUpto();
    }

    if (isUpto()) {
        return Status::Error("`UPTO' not supported yet");
    }

    return Status::OK();
}


Status GoExecutor::prepareFrom() {
    Status status = Status::OK();
    auto *clause = sentence_->fromClause();
    do {
        if (clause == nullptr) {
            LOG(FATAL) << "From clause shall never be null";
        }
        if (clause->isRef()) {
            auto *expr = clause->ref();
            if (expr->isInputExpression()) {
                auto *iexpr = static_cast<InputPropertyExpression*>(expr);
                colname_ = iexpr->prop();
            } else if (expr->isVariableExpression()) {
                auto *vexpr = static_cast<VariablePropertyExpression*>(expr);
                varname_ = vexpr->var();
                colname_ = vexpr->prop();
            } else {
                // No way to happen except memory corruption
                LOG(FATAL) << "Unknown kind of expression";
            }
            break;
        }

        auto vidList = clause->vidList();
        for (auto *expr : vidList) {
            status = expr->prepare();
            if (!status.ok()) {
                break;
            }
            auto value = expr->eval();
            if (!Expression::isInt(value)) {
                status = Status::Error("Vertex ID should be of type integer");
                break;
            }
            starts_.push_back(Expression::asInt(value));
        }
        if (!status.ok()) {
            break;
        }
    } while (false);
    return status;
}


Status GoExecutor::prepareOver() {
    Status status = Status::OK();
    auto *clause = sentence_->overClause();
    if (clause == nullptr) {
        LOG(FATAL) << "Over clause shall never be null";
    }

    auto edges = clause->edges();
    for (auto e : edges) {
        if (e->isOverAll()) {
            expCtx_->setOverAllEdge();
            return status;
        }

        auto spaceId = ectx()->rctx()->session()->space();
        auto edgeStatus = ectx()->schemaManager()->toEdgeType(spaceId, *e->edge());
        if (!edgeStatus.ok()) {
            return edgeStatus.status();
        }

        if (e->isReversely()) {
            edgeTypes_.push_back(-edgeStatus.value());
            return Status::Error("`REVERSELY' not supported yet");
        }

        auto v = edgeStatus.value();
        edgeTypes_.push_back(v);
        if (e->alias() != nullptr) {
            expCtx_->addAlias(*e->alias(), AliasKind::Edge, *e->edge(), v);
        } else {
            expCtx_->addAlias(*e->edge(), AliasKind::Edge, *e->edge(), v);
        }
    }

    return status;
}


Status GoExecutor::prepareWhere() {
    auto *clause = sentence_->whereClause();
    if (clause != nullptr) {
        filter_ = clause->filter();
    }
    return Status::OK();
}


Status GoExecutor::prepareYield() {
    auto *clause = sentence_->yieldClause();
    if (clause != nullptr) {
        yields_ = clause->columns();
    }
    return Status::OK();
}


Status GoExecutor::prepareNeededProps() {
    auto status = Status::OK();
    do {
        if (filter_ != nullptr) {
            filter_->setContext(expCtx_.get());
            status = filter_->prepare();
            if (!status.ok()) {
                break;
            }
        }
        if (yields_.empty()) {
            break;
        }
        for (auto *col : yields_) {
            col->expr()->setContext(expCtx_.get());
            status = col->expr()->prepare();
            if (!status.ok()) {
                break;
            }
        }
        if (!status.ok()) {
            break;
        }
    } while (false);

    return status;
}

Status GoExecutor::prepareDistinct() {
    auto *clause = sentence_->yieldClause();
    if (clause != nullptr) {
        distinct_ = clause->isDistinct();
        // TODO Consider distinct pushdown later, depends on filter and some other clause pushdown.
        distinctPushDown_ =
            !((expCtx_->hasSrcTagProp() || expCtx_->hasEdgeProp()) && expCtx_->hasDstTagProp());
    }
    return Status::OK();
}

Status GoExecutor::setupStarts() {
    // Literal vertex ids
    if (!starts_.empty()) {
        return Status::OK();
    }
    const auto *inputs = inputs_.get();
    // Take one column from a variable
    if (varname_ != nullptr) {
        bool existing = false;
        auto *varInputs = ectx()->variableHolder()->get(*varname_, &existing);
        if (varInputs == nullptr && !existing) {
            return Status::Error("Variable `%s' not defined", varname_->c_str());
        }
        DCHECK(inputs == nullptr);
        inputs = varInputs;
    }
    // No error happened, but we are having empty inputs
    if (inputs == nullptr) {
        return Status::OK();
    }

    auto result = inputs->getVIDs(*colname_);
    if (!result.ok()) {
        return std::move(result).status();
    }
    starts_ = std::move(result).value();
    return Status::OK();
}


void GoExecutor::setupResponse(cpp2::ExecutionResponse &resp) {
    if (resp_ == nullptr) {
        resp_ = std::make_unique<cpp2::ExecutionResponse>();
    }
    resp = std::move(*resp_);
}


void GoExecutor::stepOut() {
    auto spaceId = ectx()->rctx()->session()->space();
    auto status = getStepOutProps();
    if (!status.ok()) {
        DCHECK(onError_);
        onError_(Status::Error("Get step out props failed"));
        return;
    }
    auto returns = status.value();
    auto future  = ectx()->storage()->getNeighbors(spaceId,
                                                   starts_,
                                                   edgeTypes_,
                                                   "",
                                                   std::move(returns),
                                                   expCtx_->isOverAllEdge());
    auto *runner = ectx()->rctx()->runner();
    auto cb = [this] (auto &&result) {
        auto completeness = result.completeness();
        if (completeness == 0) {
            DCHECK(onError_);
            onError_(Status::Error("Get neighbors failed"));
            return;
        } else if (completeness != 100) {
            // TODO(dutor) We ought to let the user know that the execution was partially
            // performed, even in the case that this happened in the intermediate process.
            // Or, make this case configurable at runtime.
            // For now, we just do some logging and keep going.
            LOG(INFO) << "Get neighbors partially failed: "  << completeness << "%";
            for (auto &error : result.failedParts()) {
                LOG(ERROR) << "part: " << error.first
                           << "error code: " << static_cast<int>(error.second);
            }
        }
        onStepOutResponse(std::move(result));
    };
    auto error = [this] (auto &&e) {
        LOG(ERROR) << "Exception caught: " << e.what();
        onError_(Status::Error("Internal error"));
    };
    std::move(future).via(runner).thenValue(cb).thenError(error);
}


void GoExecutor::onStepOutResponse(RpcResponse &&rpcResp) {
    if (isFinalStep()) {
        if (expCtx_->hasDstTagProp()) {
            auto dstids = getDstIdsFromResp(rpcResp);
            if (dstids.empty()) {
                onEmptyInputs();
                return;
            }
            fetchVertexProps(std::move(dstids), std::move(rpcResp));
            return;
        }
        finishExecution(std::move(rpcResp));
        return;
    } else {
        curStep_++;
        starts_ = getDstIdsFromResp(rpcResp);
        if (starts_.empty()) {
            onEmptyInputs();
            return;
        }
        stepOut();
    }
}


void GoExecutor::onVertexProps(RpcResponse &&rpcResp) {
    UNUSED(rpcResp);
}

std::vector<std::string> GoExecutor::getEdgeNamesFromResp(RpcResponse &rpcResp) const {
    std::vector<std::string> names;
    auto spaceId = ectx()->rctx()->session()->space();
    for (auto &resp : rpcResp.responses()) {
        auto *vertices = resp.get_vertices();
        if (vertices == nullptr) {
            continue;
        }

        for (auto &vdata : *vertices) {
            for (auto &edata : vdata.edge_data) {
                auto edgeType = edata.type;
                auto status   = ectx()->schemaManager()->toEdgeName(spaceId, edgeType);
                DCHECK(status.ok());
                auto edgeName = status.value();
                names.emplace_back(std::move(edgeName));
            }
        }
    }

    return names;
}

std::vector<VertexID> GoExecutor::getDstIdsFromResp(RpcResponse &rpcResp) const {
    std::unordered_set<VertexID> set;
    for (auto &resp : rpcResp.responses()) {
        auto *vertices = resp.get_vertices();
        if (vertices == nullptr) {
            continue;
        }

        for (auto &vdata : *vertices) {
            for (auto &edata : vdata.edge_data) {
                auto schema = std::make_shared<ResultSchemaProvider>(edata.schema);
                RowSetReader rsReader(schema, edata.data);
                auto iter = rsReader.begin();
                while (iter) {
                    VertexID dst;
                    auto rc = iter->getVid("_dst", dst);
                    CHECK(rc == ResultType::SUCCEEDED);
                    set.emplace(dst);
                    ++iter;
                }
            }
        }
    }
    return std::vector<VertexID>(set.begin(), set.end());
}

void GoExecutor::finishExecution(RpcResponse &&rpcResp) {
    // MayBe we can do better.
    std::vector<std::unique_ptr<YieldColumn>> yc;
    if (expCtx_->isOverAllEdge() && yields_.empty()) {
        auto edgeNames = getEdgeNamesFromResp(rpcResp);
        for (const auto &name : edgeNames) {
            auto dummy       = new std::string(name);
            auto dummy_alias = new std::string(name + "_id");
            auto dummy_exp   = new EdgeDstIdExpression(dummy);
            auto ptr         = std::make_unique<YieldColumn>(dummy_exp, dummy_alias);
            dummy_exp->setContext(expCtx_.get());
            yields_.emplace_back(ptr.get());
            yc.emplace_back(std::move(ptr));
        }
    }

    auto outputs = setupInterimResult(std::move(rpcResp));
    if (onResult_) {
        onResult_(std::move(outputs));
    } else {
        resp_ = std::make_unique<cpp2::ExecutionResponse>();
        resp_->set_column_names(getResultColumnNames());
        if (outputs != nullptr) {
            auto rows = outputs->getRows();
            resp_->set_rows(std::move(rows));
        }
    }
    DCHECK(onFinish_);
    onFinish_();
}

StatusOr<std::vector<storage::cpp2::PropDef>> GoExecutor::getStepOutProps() const {
    std::vector<storage::cpp2::PropDef> props;
    for (auto &e : edgeTypes_) {
        storage::cpp2::PropDef pd;
        pd.owner = storage::cpp2::PropOwner::EDGE;
        pd.name = "_dst";
        pd.id.set_edge_type(e);
        props.emplace_back(std::move(pd));
    }

    if (!isFinalStep()) {
        return props;
    }

    auto spaceId = ectx()->rctx()->session()->space();
    for (auto &tagProp : expCtx_->srcTagProps()) {
        storage::cpp2::PropDef pd;
        pd.owner = storage::cpp2::PropOwner::SOURCE;
        pd.name = tagProp.second;
        auto status = ectx()->schemaManager()->toTagID(spaceId, tagProp.first);
        if (!status.ok()) {
            return Status::Error("No schema found for '%s'", tagProp.first);
        }
        auto tagId = status.value();
        pd.id.set_tag_id(tagId);
        props.emplace_back(std::move(pd));
    }

    for (auto &prop : expCtx_->edgeProps()) {
        storage::cpp2::PropDef pd;
        pd.owner = storage::cpp2::PropOwner::EDGE;
        pd.name = prop.first;
        if (expCtx_->isOverAllEdge()) {
            auto edgeName = boost::get<std::string>(prop.second);

            auto status = ectx()->schemaManager()->toEdgeType(spaceId, edgeName);
            if (!status.ok()) {
                return Status::Error("No schema found for '%s'",
                                     boost::get<std::string>(prop.second));
            }
            auto edgeType = status.value();
            pd.id.set_edge_type(edgeType);
            expCtx_->addAlias(edgeName, AliasKind::Edge, edgeName, edgeType);
        } else {
            pd.id.set_edge_type(boost::get<EdgeType>(prop.second));
        }
        props.emplace_back(std::move(pd));
    }

    return props;
}


StatusOr<std::vector<storage::cpp2::PropDef>> GoExecutor::getDstProps() const {
    std::vector<storage::cpp2::PropDef> props;
    auto spaceId = ectx()->rctx()->session()->space();
    for (auto &tagProp : expCtx_->dstTagProps()) {
        storage::cpp2::PropDef pd;
        pd.owner = storage::cpp2::PropOwner::DEST;
        pd.name = tagProp.second;
        auto status = ectx()->schemaManager()->toTagID(spaceId, tagProp.first);
        if (!status.ok()) {
            return Status::Error("No schema found for '%s'", tagProp.first);
        }
        auto tagId = status.value();
        pd.id.set_tag_id(tagId);
        props.emplace_back(std::move(pd));
    }
    return props;
}


void GoExecutor::fetchVertexProps(std::vector<VertexID> ids, RpcResponse &&rpcResp) {
    auto spaceId = ectx()->rctx()->session()->space();
    auto status = getDstProps();
    if (!status.ok()) {
        DCHECK(onError_);
        onError_(Status::Error("Get dest props failed"));
        return;
    }
    auto returns = status.value();
    auto future = ectx()->storage()->getVertexProps(spaceId, ids, returns);
    auto *runner = ectx()->rctx()->runner();
    auto cb = [this, stepOutResp = std::move(rpcResp)] (auto &&result) mutable {
        auto completeness = result.completeness();
        if (completeness == 0) {
            DCHECK(onError_);
            onError_(Status::Error("Get dest props failed"));
            return;
        } else if (completeness != 100) {
            LOG(INFO) << "Get neighbors partially failed: "  << completeness << "%";
            for (auto &error : result.failedParts()) {
                LOG(ERROR) << "part: " << error.first
                           << "error code: " << static_cast<int>(error.second);
            }
        }
        if (vertexHolder_ == nullptr) {
            vertexHolder_ = std::make_unique<VertexHolder>();
        }
        for (auto &resp : result.responses()) {
            vertexHolder_->add(resp);
        }
        finishExecution(std::move(stepOutResp));
        return;
    };
    auto error = [this] (auto &&e) {
        LOG(ERROR) << "Exception caught: " << e.what();
        onError_(Status::Error("Internal error"));
    };
    std::move(future).via(runner).thenValue(cb).thenError(error);
}


std::vector<std::string> GoExecutor::getResultColumnNames() const {
    std::vector<std::string> result;
    result.reserve(yields_.size());
    for (auto *col : yields_) {
        if (col->alias() == nullptr) {
            result.emplace_back(col->expr()->toString());
        } else {
            result.emplace_back(*col->alias());
        }
    }
    return result;
}


std::unique_ptr<InterimResult> GoExecutor::setupInterimResult(RpcResponse &&rpcResp) {
    // Generic results
    std::shared_ptr<SchemaWriter> schema;
    std::unique_ptr<RowSetWriter> rsWriter;
    auto uniqResult = std::make_unique<std::unordered_set<std::string>>();
    using nebula::cpp2::SupportedType;
    auto cb = [&] (std::vector<VariantType> record) {
        if (schema == nullptr) {
            schema = std::make_shared<SchemaWriter>();
            auto colnames = getResultColumnNames();
            for (auto i = 0u; i < record.size(); i++) {
                SupportedType type;
                switch (record[i].which()) {
                    case 0:
                        // all integers in InterimResult are regarded as type of VID
                        type = SupportedType::VID;
                        break;
                    case 1:
                        type = SupportedType::DOUBLE;
                        break;
                    case 2:
                        type = SupportedType::BOOL;
                        break;
                    case 3:
                        type = SupportedType::STRING;
                        break;
                    default:
                        LOG(FATAL) << "Unknown VariantType: " << record[i].which();
                }
                schema->appendCol(colnames[i], type);
            }   // for
            rsWriter = std::make_unique<RowSetWriter>(schema);
        }   // if

        RowWriter writer(schema);
        for (auto &column : record) {
            switch (column.which()) {
                case 0:
                    writer << boost::get<int64_t>(column);
                    break;
                case 1:
                    writer << boost::get<double>(column);
                    break;
                case 2:
                    writer << boost::get<bool>(column);
                    break;
                case 3:
                    writer << boost::get<std::string>(column);
                    break;
                case 4:
                    writer << "";
                default:
                    LOG(FATAL) << "Unknown VariantType: " << column.which();
            }
        }
        // TODO Consider float/double, and need to reduce mem copy.
        std::string encode = writer.encode();
        if (distinct_) {
            auto ret = uniqResult->emplace(encode);
            if (ret.second) {
                rsWriter->addRow(writer);
            }
        } else {
            rsWriter->addRow(writer);
        }
    };  // cb
    processFinalResult(rpcResp, cb);
    // No results populated
    if (rsWriter == nullptr) {
        return nullptr;
    }
    return std::make_unique<InterimResult>(std::move(rsWriter));
}

void GoExecutor::onEmptyInputs() {
    if (onResult_) {
        onResult_(nullptr);
    } else if (resp_ == nullptr) {
        resp_ = std::make_unique<cpp2::ExecutionResponse>();
    }
    onFinish_();
}


void GoExecutor::processFinalResult(RpcResponse &rpcResp, Callback cb) const {
    auto all = rpcResp.responses();
    auto spaceId = ectx()->rctx()->session()->space();
    for (auto &resp : all) {
        if (resp.get_vertices() == nullptr) {
            continue;
        }
        std::shared_ptr<ResultSchemaProvider> vschema;

        if (resp.get_vertex_schema() != nullptr) {
            vschema = std::make_shared<ResultSchemaProvider>(resp.vertex_schema);
        }

        for (auto &vdata : resp.vertices) {
            std::unique_ptr<RowReader> vreader;
            if (vschema != nullptr) {
                DCHECK(vdata.__isset.vertex_data);
                vreader = RowReader::getRowReader(vdata.vertex_data, vschema);
            }
            DCHECK(vdata.__isset.edge_data);
            for (auto &edata : vdata.edge_data) {
                std::shared_ptr<ResultSchemaProvider> eschema;

                eschema = std::make_shared<ResultSchemaProvider>(edata.schema);

                DCHECK(eschema != nullptr);
                RowSetReader rsReader(eschema, edata.data);
                auto iter = rsReader.begin();
                auto edgeType = edata.type;
                while (iter) {
                    auto &getters = expCtx_->getters();

                    getters.getEdgeProp = [&iter, &spaceId, &edgeType, this](
                                              const std::string &edgeName,
                                              const std::string &prop) -> VariantType {
                        auto edgeStatus = ectx()->schemaManager()->toEdgeType(spaceId, edgeName);
                        CHECK(edgeStatus.ok());

                        if (edgeType != edgeStatus.value()) {
                            return RowReader::getDefaultProp(iter->getSchema(), prop);
                        }
                        auto res = RowReader::getProp(&*iter, prop);
                        CHECK(ok(res));
                        return value(std::move(res));
                    };
                    getters.getSrcTagProp = [&vreader](const std::string &,
                                                       const std::string &prop) {
                        auto res = RowReader::getProp(vreader.get(), prop);
                        CHECK(ok(res));
                        return value(std::move(res));
                    };
                    getters.getDstTagProp = [&iter, &spaceId, this](const std::string &tag,
                                                                    const std::string &prop) {
                        auto dst = RowReader::getProp(&*iter, "_dst");
                        CHECK(ok(dst));
                        auto vid    = boost::get<int64_t>(value(std::move(dst)));
                        auto status = ectx()->schemaManager()->toTagID(spaceId, tag);
                        CHECK(status.ok());
                        auto tagId = status.value();
                        if (!vertexHolder_->exist(vid, tagId)) {
                            return vertexHolder_->getDefaultProp(prop);
                        }
                        return vertexHolder_->get(vid, prop);
                    };
                    // Evaluate filter
                    if (filter_ != nullptr) {
                        auto value = filter_->eval();
                        if (!Expression::asBool(value)) {
                            ++iter;
                            continue;
                        }
                    }

                    std::vector<VariantType> record;
                    record.reserve(yields_.size());
                    for (auto *column : yields_) {
                        auto *expr = column->expr();
                        // TODO(dutor) `eval' may fail
                        auto value = expr->eval();
                        record.emplace_back(std::move(value));
                    }
                    cb(std::move(record));
                    ++iter;
                }  // while `iter'
            }
        }   // for `vdata'
    }   // for `resp'
}

VariantType GoExecutor::VertexHolder::getDefaultProp(const std::string &prop) const {
    DCHECK(schema_ != nullptr);

    return RowReader::getDefaultProp(schema_.get(), prop);
}

bool GoExecutor::VertexHolder::exist(VertexID vid, TagID tid) const {
    auto iter = data_.find(vid);
    CHECK(iter != data_.end());
    auto &v = std::get<0>(iter->second);
    auto it = std::find(v.cbegin(), v.cend(), tid);
    return it != v.cend();
}

VariantType GoExecutor::VertexHolder::get(VertexID id, const std::string &prop) const {
    DCHECK(schema_ != nullptr);
    auto iter = data_.find(id);

    // TODO(dutor) We need a type to represent NULL or non-existing prop
    CHECK(iter != data_.end());

    auto reader = RowReader::getRowReader(std::get<1>(iter->second), schema_);

    auto res = RowReader::getProp(reader.get(), prop);
    CHECK(ok(res));
    return value(std::move(res));
}


void GoExecutor::VertexHolder::add(const storage::cpp2::QueryResponse &resp) {
    auto *vertices = resp.get_vertices();
    if (vertices == nullptr) {
        return;
    }
    if (resp.get_vertex_schema() == nullptr) {
        return;
    }
    if (schema_ == nullptr) {
        schema_ = std::make_shared<ResultSchemaProvider>(resp.vertex_schema);
    }

    for (auto &vdata : *vertices) {
        DCHECK(vdata.__isset.vertex_data);
        data_[vdata.vertex_id] = std::make_tuple(vdata.tag_ids, vdata.vertex_data);
    }
}

}   // namespace graph
}   // namespace nebula
