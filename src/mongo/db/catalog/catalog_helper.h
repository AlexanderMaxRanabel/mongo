/**
 *    Copyright (C) 2022-present MongoDB, Inc.
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the Server Side Public License, version 1,
 *    as published by MongoDB, Inc.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    Server Side Public License for more details.
 *
 *    You should have received a copy of the Server Side Public License
 *    along with this program. If not, see
 *    <http://www.mongodb.com/licensing/server-side-public-license>.
 *
 *    As a special exception, the copyright holders give permission to link the
 *    code of portions of this program with the OpenSSL library under certain
 *    conditions as described in each individual source file and distribute
 *    linked combinations including the program with the OpenSSL library. You
 *    must comply with the Server Side Public License in all respects for
 *    all of the code used other than as permitted herein. If you modify file(s)
 *    with this exception, you may extend this exception to your version of the
 *    file(s), but you are not obligated to do so. If you do not wish to do so,
 *    delete this exception statement from your version. If you delete this
 *    exception statement from all source files in the program, then also delete
 *    it in the license file.
 */

#pragma once

#include "mongo/base/string_data.h"
#include "mongo/db/catalog_raii.h"
#include "mongo/db/operation_context.h"
#include "mongo/s/database_version.h"

namespace mongo::catalog_helper {

/**
 * Checks that the cached database version matches the one attached to the operation, which means
 * that the operation is routed to the right shard (database owner).
 *
 * Throws `StaleDbRoutingVersion` exception when the critical section is taken, there is no cached
 * database version, or the cached database version does not match the one sent by the client.
 */
void assertMatchingDbVersion(OperationContext* opCtx, const StringData& dbName);

/**
 * Checks that the current shard server is the primary for the given database, throwing
 * `IllegalOperation` if it is not.
 */
void assertIsPrimaryShardForDb(OperationContext* opCtx, const StringData& dbName);

/**
 * Fills the input 'collLocks' with CollectionLocks, acquiring locks on namespaces 'nsOrUUID' and
 * 'secondaryNssOrUUIDs' in ResourceId(RESOURCE_COLLECTION, nss) order.
 *
 * The namespaces will be resolved, the locks acquired, and then the namespaces will be checked for
 * changes in case there is a race with rename and a UUID no longer matches the locked namespace.
 *
 * Handles duplicate namespaces across 'nsOrUUID' and 'secondaryNssOrUUIDs'. Only one lock will be
 * taken on each namespace.
 */
void acquireCollectionLocksInResourceIdOrder(
    OperationContext* opCtx,
    const NamespaceStringOrUUID& nsOrUUID,
    LockMode modeColl,
    Date_t deadline,
    const std::vector<NamespaceStringOrUUID>& secondaryNssOrUUIDs,
    std::vector<CollectionNamespaceOrUUIDLock>* collLocks);

/**
 * Executes the provided callback on the 'setAutoGetCollectionWait' FailPoint.
 */
void setAutoGetCollectionWaitFailpointExecute(std::function<void(const BSONObj&)> callback);

}  // namespace mongo::catalog_helper
