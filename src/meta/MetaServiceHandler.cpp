/* Copyright (c) 2018 vesoft inc. All rights reserved.
 *
 * This source code is licensed under Apache 2.0 License,
 * attached with Common Clause Condition 1.0, found in the LICENSES directory.
 */

#include "meta/MetaServiceUtils.h"
#include "meta/MetaServiceHandler.h"
#include "meta/processors/partsMan/CreateSpaceProcessor.h"
#include "meta/processors/partsMan/DropSpaceProcessor.h"
#include "meta/processors/partsMan/ListSpacesProcessor.h"
#include "meta/processors/partsMan/GetSpaceProcessor.h"
#include "meta/processors/partsMan/ListHostsProcessor.h"
#include "meta/processors/partsMan/ListPartsProcessor.h"
#include "meta/processors/partsMan/GetPartsAllocProcessor.h"
#include "meta/processors/schemaMan/CreateTagProcessor.h"
#include "meta/processors/schemaMan/AlterTagProcessor.h"
#include "meta/processors/schemaMan/DropTagProcessor.h"
#include "meta/processors/schemaMan/GetTagProcessor.h"
#include "meta/processors/schemaMan/ListTagsProcessor.h"
#include "meta/processors/schemaMan/CreateEdgeProcessor.h"
#include "meta/processors/schemaMan/AlterEdgeProcessor.h"
#include "meta/processors/schemaMan/DropEdgeProcessor.h"
#include "meta/processors/schemaMan/GetEdgeProcessor.h"
#include "meta/processors/schemaMan/ListEdgesProcessor.h"
#include "meta/processors/indexMan/CreateTagIndexProcessor.h"
#include "meta/processors/indexMan/DropTagIndexProcessor.h"
#include "meta/processors/indexMan/GetTagIndexProcessor.h"
#include "meta/processors/indexMan/ListTagIndexesProcessor.h"
#include "meta/processors/indexMan/RebuildTagIndexProcessor.h"
#include "meta/processors/indexMan/ListTagIndexStatusProcessor.h"
#include "meta/processors/indexMan/CreateEdgeIndexProcessor.h"
#include "meta/processors/indexMan/DropEdgeIndexProcessor.h"
#include "meta/processors/indexMan/GetEdgeIndexProcessor.h"
#include "meta/processors/indexMan/ListEdgeIndexesProcessor.h"
#include "meta/processors/indexMan/RebuildEdgeIndexProcessor.h"
#include "meta/processors/indexMan/ListEdgeIndexStatusProcessor.h"
#include "meta/processors/customKV/MultiPutProcessor.h"
#include "meta/processors/customKV/GetProcessor.h"
#include "meta/processors/customKV/MultiGetProcessor.h"
#include "meta/processors/customKV/ScanProcessor.h"
#include "meta/processors/customKV/RemoveProcessor.h"
#include "meta/processors/customKV/RemoveRangeProcessor.h"
#include "meta/processors/admin/HBProcessor.h"
#include "meta/processors/usersMan/AuthenticationProcessor.h"
#include "meta/processors/admin/BalanceProcessor.h"
#include "meta/processors/admin/LeaderBalanceProcessor.h"
#include "meta/processors/admin/CreateSnapshotProcessor.h"
#include "meta/processors/admin/DropSnapshotProcessor.h"
#include "meta/processors/admin/ListSnapshotsProcessor.h"
#include "meta/processors/configMan/RegConfigProcessor.h"
#include "meta/processors/configMan/GetConfigProcessor.h"
#include "meta/processors/configMan/SetConfigProcessor.h"
#include "meta/processors/configMan/ListConfigsProcessor.h"
#include "meta/processors/jobMan/AdminJobProcessor.h"

#define RETURN_FUTURE(processor) \
    auto f = processor->getFuture(); \
    processor->process(req); \
    return f;

namespace nebula {
namespace meta {

folly::Future<cpp2::ExecResp>
MetaServiceHandler::future_createSpace(const cpp2::CreateSpaceReq& req) {
    auto* processor = CreateSpaceProcessor::instance(kvstore_);
    RETURN_FUTURE(processor);
}

folly::Future<cpp2::ExecResp>
MetaServiceHandler::future_dropSpace(const cpp2::DropSpaceReq& req) {
    auto* processor = DropSpaceProcessor::instance(kvstore_);
    RETURN_FUTURE(processor);
}

folly::Future<cpp2::ListSpacesResp>
MetaServiceHandler::future_listSpaces(const cpp2::ListSpacesReq& req) {
    auto* processor = ListSpacesProcessor::instance(kvstore_);
    RETURN_FUTURE(processor);
}

folly::Future<cpp2::AdminJobResp>
MetaServiceHandler::future_runAdminJob(const cpp2::AdminJobReq& req) {
    auto* processor = AdminJobProcessor::instance(kvstore_);
    RETURN_FUTURE(processor);
}

folly::Future<cpp2::GetSpaceResp>
MetaServiceHandler::future_getSpace(const cpp2::GetSpaceReq& req) {
    auto* processor = GetSpaceProcessor::instance(kvstore_);
    RETURN_FUTURE(processor);
}

folly::Future<cpp2::ListHostsResp>
MetaServiceHandler::future_listHosts(const cpp2::ListHostsReq& req) {
    auto* processor = ListHostsProcessor::instance(kvstore_);
    RETURN_FUTURE(processor);
}

folly::Future<cpp2::ListPartsResp>
MetaServiceHandler::future_listParts(const cpp2::ListPartsReq& req) {
    auto* processor = ListPartsProcessor::instance(kvstore_);
    RETURN_FUTURE(processor);
}

folly::Future<cpp2::GetPartsAllocResp>
MetaServiceHandler::future_getPartsAlloc(const cpp2::GetPartsAllocReq& req) {
    auto* processor = GetPartsAllocProcessor::instance(kvstore_);
    RETURN_FUTURE(processor);
}

folly::Future<cpp2::ExecResp>
MetaServiceHandler::future_multiPut(const cpp2::MultiPutReq& req) {
    auto* processor = MultiPutProcessor::instance(kvstore_);
    RETURN_FUTURE(processor);
}

folly::Future<cpp2::GetResp>
MetaServiceHandler::future_get(const cpp2::GetReq& req) {
    auto* processor = GetProcessor::instance(kvstore_);
    RETURN_FUTURE(processor);
}

folly::Future<cpp2::MultiGetResp>
MetaServiceHandler::future_multiGet(const cpp2::MultiGetReq& req) {
    auto* processor = MultiGetProcessor::instance(kvstore_);
    RETURN_FUTURE(processor);
}

folly::Future<cpp2::ScanResp>
MetaServiceHandler::future_scan(const cpp2::ScanReq& req) {
    auto* processor = ScanProcessor::instance(kvstore_);
    RETURN_FUTURE(processor);
}

folly::Future<cpp2::ExecResp>
MetaServiceHandler::future_remove(const cpp2::RemoveReq& req) {
    auto* processor = RemoveProcessor::instance(kvstore_);
    RETURN_FUTURE(processor);
}

folly::Future<cpp2::ExecResp>
MetaServiceHandler::future_removeRange(const cpp2::RemoveRangeReq& req) {
    auto* processor = RemoveRangeProcessor::instance(kvstore_);
    RETURN_FUTURE(processor);
}

folly::Future<cpp2::ExecResp>
MetaServiceHandler::future_createTag(const cpp2::CreateTagReq& req) {
    auto* processor = CreateTagProcessor::instance(kvstore_);
    RETURN_FUTURE(processor);
}

folly::Future<cpp2::ExecResp>
MetaServiceHandler::future_alterTag(const cpp2::AlterTagReq& req) {
    auto* processor = AlterTagProcessor::instance(kvstore_);
    RETURN_FUTURE(processor);
}

folly::Future<cpp2::ExecResp>
MetaServiceHandler::future_dropTag(const cpp2::DropTagReq& req) {
    auto* processor = DropTagProcessor::instance(kvstore_);
    RETURN_FUTURE(processor);
}

folly::Future<cpp2::GetTagResp>
MetaServiceHandler::future_getTag(const cpp2::GetTagReq &req) {
    auto* processor = GetTagProcessor::instance(kvstore_);
    RETURN_FUTURE(processor);
}

folly::Future<cpp2::ListTagsResp>
MetaServiceHandler::future_listTags(const cpp2::ListTagsReq& req) {
    auto* processor = ListTagsProcessor::instance(kvstore_);
    RETURN_FUTURE(processor);
}

folly::Future<cpp2::ExecResp>
MetaServiceHandler::future_createEdge(const cpp2::CreateEdgeReq& req) {
    auto* processor = CreateEdgeProcessor::instance(kvstore_);
    RETURN_FUTURE(processor);
}

folly::Future<cpp2::ExecResp>
MetaServiceHandler::future_alterEdge(const cpp2::AlterEdgeReq& req) {
    auto* processor = AlterEdgeProcessor::instance(kvstore_);
    RETURN_FUTURE(processor);
}

folly::Future<cpp2::ExecResp>
MetaServiceHandler::future_dropEdge(const cpp2::DropEdgeReq& req) {
    auto* processor = DropEdgeProcessor::instance(kvstore_);
    RETURN_FUTURE(processor);
}

folly::Future<cpp2::GetEdgeResp>
MetaServiceHandler::future_getEdge(const cpp2::GetEdgeReq& req) {
    auto* processor = GetEdgeProcessor::instance(kvstore_);
    RETURN_FUTURE(processor);
}

folly::Future<cpp2::ListEdgesResp>
MetaServiceHandler::future_listEdges(const cpp2::ListEdgesReq& req) {
    auto* processor = ListEdgesProcessor::instance(kvstore_);
    RETURN_FUTURE(processor);
}

folly::Future<cpp2::ExecResp>
MetaServiceHandler::future_createTagIndex(const cpp2::CreateTagIndexReq& req) {
    auto* processor = CreateTagIndexProcessor::instance(kvstore_);
    RETURN_FUTURE(processor);
}

folly::Future<cpp2::ExecResp>
MetaServiceHandler::future_dropTagIndex(const cpp2::DropTagIndexReq& req) {
    auto* processor = DropTagIndexProcessor::instance(kvstore_);
    RETURN_FUTURE(processor);
}

folly::Future<cpp2::GetTagIndexResp>
MetaServiceHandler::future_getTagIndex(const cpp2::GetTagIndexReq &req) {
    auto* processor = GetTagIndexProcessor::instance(kvstore_);
    RETURN_FUTURE(processor);
}

folly::Future<cpp2::ListTagIndexesResp>
MetaServiceHandler::future_listTagIndexes(const cpp2::ListTagIndexesReq& req) {
    auto* processor = ListTagIndexesProcessor::instance(kvstore_);
    RETURN_FUTURE(processor);
}

folly::Future<cpp2::ExecResp>
MetaServiceHandler::future_rebuildTagIndex(const cpp2::RebuildIndexReq& req) {
    auto* processor = RebuildTagIndexProcessor::instance(kvstore_, adminClient_.get());
    RETURN_FUTURE(processor);
}

folly::Future<cpp2::ListIndexStatusResp>
MetaServiceHandler::future_listTagIndexStatus(const cpp2::ListIndexStatusReq& req) {
    auto* processor = ListTagIndexStatusProcessor::instance(kvstore_);
    RETURN_FUTURE(processor);
}

folly::Future<cpp2::ExecResp>
MetaServiceHandler::future_createEdgeIndex(const cpp2::CreateEdgeIndexReq& req) {
    auto* processor = CreateEdgeIndexProcessor::instance(kvstore_);
    RETURN_FUTURE(processor);
}

folly::Future<cpp2::ExecResp>
MetaServiceHandler::future_dropEdgeIndex(const cpp2::DropEdgeIndexReq& req) {
    auto* processor = DropEdgeIndexProcessor::instance(kvstore_);
    RETURN_FUTURE(processor);
}

folly::Future<cpp2::GetEdgeIndexResp>
MetaServiceHandler::future_getEdgeIndex(const cpp2::GetEdgeIndexReq& req) {
    auto* processor = GetEdgeIndexProcessor::instance(kvstore_);
    RETURN_FUTURE(processor);
}

folly::Future<cpp2::ListEdgeIndexesResp>
MetaServiceHandler::future_listEdgeIndexes(const cpp2::ListEdgeIndexesReq& req) {
    auto* processor = ListEdgeIndexesProcessor::instance(kvstore_);
    RETURN_FUTURE(processor);
}

folly::Future<cpp2::ExecResp>
MetaServiceHandler::future_rebuildEdgeIndex(const cpp2::RebuildIndexReq& req) {
    auto* processor = RebuildEdgeIndexProcessor::instance(kvstore_, adminClient_.get());
    RETURN_FUTURE(processor);
}

folly::Future<cpp2::ListIndexStatusResp>
MetaServiceHandler::future_listEdgeIndexStatus(const cpp2::ListIndexStatusReq& req) {
    auto* processor = ListEdgeIndexStatusProcessor::instance(kvstore_);
    RETURN_FUTURE(processor);
}

folly::Future<cpp2::HBResp>
MetaServiceHandler::future_heartBeat(const cpp2::HBReq& req) {
    auto* processor = HBProcessor::instance(kvstore_, clusterId_, &heartBeatStat_);
    RETURN_FUTURE(processor);
}

folly::Future<cpp2::ExecResp>
MetaServiceHandler::future_createUser(const cpp2::CreateUserReq& req) {
    auto* processor = CreateUserProcessor::instance(kvstore_);
    RETURN_FUTURE(processor);
}

folly::Future<cpp2::ExecResp>
MetaServiceHandler::future_dropUser(const cpp2::DropUserReq& req) {
    auto* processor = DropUserProcessor::instance(kvstore_);
    RETURN_FUTURE(processor);
}

folly::Future<cpp2::ExecResp>
MetaServiceHandler::future_alterUser(const cpp2::AlterUserReq& req) {
    auto* processor = AlterUserProcessor::instance(kvstore_);
    RETURN_FUTURE(processor);
}

folly::Future<cpp2::ExecResp>
MetaServiceHandler::future_grantRole(const cpp2::GrantRoleReq& req) {
    auto* processor = GrantProcessor::instance(kvstore_);
    RETURN_FUTURE(processor);
}

folly::Future<cpp2::ExecResp>
MetaServiceHandler::future_revokeRole(const cpp2::RevokeRoleReq& req) {
    auto* processor = RevokeProcessor::instance(kvstore_);
    RETURN_FUTURE(processor);
}

folly::Future<cpp2::ListUsersResp>
MetaServiceHandler::future_listUsers(const cpp2::ListUsersReq& req) {
    auto* processor = ListUsersProcessor::instance(kvstore_);
    RETURN_FUTURE(processor);
}

folly::Future<cpp2::ListRolesResp>
MetaServiceHandler::future_listRoles(const cpp2::ListRolesReq& req) {
    auto* processor = ListRolesProcessor::instance(kvstore_);
    RETURN_FUTURE(processor);
}

folly::Future<cpp2::ExecResp>
MetaServiceHandler::future_changePassword(const cpp2::ChangePasswordReq& req) {
    auto* processor = ChangePasswordProcessor::instance(kvstore_);
    RETURN_FUTURE(processor);
}

folly::Future<cpp2::ListRolesResp>
MetaServiceHandler::future_getUserRoles(const cpp2::GetUserRolesReq& req) {
    auto* processor = GetUserRolesProcessor::instance(kvstore_);
    RETURN_FUTURE(processor);
}

folly::Future<cpp2::BalanceResp>
MetaServiceHandler::future_balance(const cpp2::BalanceReq& req) {
    auto* processor = BalanceProcessor::instance(kvstore_);
    RETURN_FUTURE(processor);
}

folly::Future<cpp2::ExecResp>
MetaServiceHandler::future_leaderBalance(const cpp2::LeaderBalanceReq& req) {
    auto* processor = LeaderBalanceProcessor::instance(kvstore_);
    RETURN_FUTURE(processor);
}

folly::Future<cpp2::ExecResp>
MetaServiceHandler::future_regConfig(const cpp2::RegConfigReq &req) {
    auto* processor = RegConfigProcessor::instance(kvstore_);
    RETURN_FUTURE(processor);
}

folly::Future<cpp2::GetConfigResp>
MetaServiceHandler::future_getConfig(const cpp2::GetConfigReq &req) {
    auto* processor = GetConfigProcessor::instance(kvstore_);
    RETURN_FUTURE(processor);
}

folly::Future<cpp2::ExecResp>
MetaServiceHandler::future_setConfig(const cpp2::SetConfigReq &req) {
    auto* processor = SetConfigProcessor::instance(kvstore_);
    RETURN_FUTURE(processor);
}

folly::Future<cpp2::ListConfigsResp>
MetaServiceHandler::future_listConfigs(const cpp2::ListConfigsReq &req) {
    auto* processor = ListConfigsProcessor::instance(kvstore_);
    RETURN_FUTURE(processor);
}

folly::Future<cpp2::ExecResp>
MetaServiceHandler::future_createSnapshot(const cpp2::CreateSnapshotReq& req) {
    auto* processor = CreateSnapshotProcessor::instance(kvstore_);
    RETURN_FUTURE(processor);
}

folly::Future<cpp2::ExecResp>
MetaServiceHandler::future_dropSnapshot(const cpp2::DropSnapshotReq& req) {
    auto* processor = DropSnapshotProcessor::instance(kvstore_);
    RETURN_FUTURE(processor);
}

folly::Future<cpp2::ListSnapshotsResp>
MetaServiceHandler::future_listSnapshots(const cpp2::ListSnapshotsReq& req) {
    auto* processor = ListSnapshotsProcessor::instance(kvstore_);
    RETURN_FUTURE(processor);
}

}  // namespace meta
}  // namespace nebula
