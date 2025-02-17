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

#include "mongo/platform/basic.h"

#include "mongo/crypto/fle_stats.h"

#include "mongo/bson/unordered_fields_bsonobj_comparator.h"
#include "mongo/db/operation_context_noop.h"
#include "mongo/unittest/unittest.h"
#include "mongo/util/testing_options_gen.h"
#include "mongo/util/tick_source_mock.h"

#define MONGO_LOGV2_DEFAULT_COMPONENT ::mongo::logv2::LogComponent::kTest

namespace mongo {

class FLEStatsTest : public unittest::Test {
public:
    void setUp() final {
        oldDiagnosticsFlag = gTestingDiagnosticsEnabledAtStartup;
        tickSource = std::make_unique<TickSourceMock<Milliseconds>>();
        instance = std::make_unique<FLEStatusSection>(tickSource.get());
    }

    void tearDown() final {
        gTestingDiagnosticsEnabledAtStartup = oldDiagnosticsFlag;
    }

protected:
    CompactStats zeroStats = CompactStats::parse(
        IDLParserContext("compactStats"),
        BSON("ecoc" << BSON("deleted" << 0 << "read" << 0) << "ecc"
                    << BSON("deleted" << 0 << "inserted" << 0 << "read" << 0 << "updated" << 0)
                    << "esc"
                    << BSON("deleted" << 0 << "inserted" << 0 << "read" << 0 << "updated" << 0)));

    CompactStats compactStats = CompactStats::parse(
        IDLParserContext("compactStats"),
        BSON("ecoc" << BSON("deleted" << 1 << "read" << 1) << "ecc"
                    << BSON("deleted" << 1 << "inserted" << 1 << "read" << 1 << "updated" << 1)
                    << "esc"
                    << BSON("deleted" << 1 << "inserted" << 1 << "read" << 1 << "updated" << 1)));
    std::unique_ptr<TickSourceMock<Milliseconds>> tickSource;
    std::unique_ptr<FLEStatusSection> instance;
    OperationContextNoop opCtx;
    bool oldDiagnosticsFlag;
};

TEST_F(FLEStatsTest, NoopStats) {
    ASSERT_FALSE(instance->includeByDefault());

    auto obj = instance->generateSection(&opCtx, BSONElement());
    ASSERT_TRUE(obj.hasField("compactStats"));
    ASSERT_BSONOBJ_EQ(zeroStats.toBSON(), obj["compactStats"].Obj());
    ASSERT_FALSE(obj.hasField("emuBinaryStats"));
}

TEST_F(FLEStatsTest, CompactStats) {
    instance->updateCompactionStats(compactStats);

    ASSERT_TRUE(instance->includeByDefault());

    auto obj = instance->generateSection(&opCtx, BSONElement());
    ASSERT_TRUE(obj.hasField("compactStats"));
    ASSERT_BSONOBJ_NE(zeroStats.toBSON(), obj["compactStats"].Obj());
    ASSERT_BSONOBJ_EQ(compactStats.toBSON(), obj["compactStats"].Obj());
    ASSERT_FALSE(obj.hasField("emuBinaryStats"));
}

TEST_F(FLEStatsTest, BinaryEmuStatsAreEmptyWithoutTesting) {
    {
        auto tracker = instance->makeEmuBinaryTracker();
        tracker.recordSuboperation();
    }

    ASSERT_FALSE(instance->includeByDefault());

    auto obj = instance->generateSection(&opCtx, BSONElement());
    ASSERT_TRUE(obj.hasField("compactStats"));
    ASSERT_BSONOBJ_EQ(zeroStats.toBSON(), obj["compactStats"].Obj());
    ASSERT_FALSE(obj.hasField("emuBinaryStats"));
}

TEST_F(FLEStatsTest, BinaryEmuStatsArePopulatedWithTesting) {
    gTestingDiagnosticsEnabledAtStartup = true;

    {
        auto tracker = instance->makeEmuBinaryTracker();
        tracker.recordSuboperation();
        tickSource->advance(Milliseconds(100));
    }

    ASSERT_TRUE(instance->includeByDefault());

    auto obj = instance->generateSection(&opCtx, BSONElement());
    ASSERT_TRUE(obj.hasField("compactStats"));
    ASSERT_BSONOBJ_EQ(zeroStats.toBSON(), obj["compactStats"].Obj());
    ASSERT_TRUE(obj.hasField("emuBinaryStats"));
    ASSERT_EQ(1, obj["emuBinaryStats"]["calls"].Long());
    ASSERT_EQ(1, obj["emuBinaryStats"]["suboperations"].Long());
    ASSERT_EQ(100, obj["emuBinaryStats"]["totalMillis"].Long());
}


}  // namespace mongo
