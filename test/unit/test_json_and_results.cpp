#include "json_helper.hpp"
#include "pricing_results.hpp"

#include "pnl_test_utils.hpp"

#include <gtest/gtest.h>

#include <sstream>
#include <string>

TEST(JsonHelperTest, SerializesVectorToJsonArray)
{
    auto vector = makeVector({1.25, -2.5, 3.75});

    nlohmann::json json = vector.get();

    ASSERT_TRUE(json.is_array());
    ASSERT_EQ(json.size(), 3u);
    EXPECT_DOUBLE_EQ(json.at(0).get<double>(), 1.25);
    EXPECT_DOUBLE_EQ(json.at(1).get<double>(), -2.5);
    EXPECT_DOUBLE_EQ(json.at(2).get<double>(), 3.75);
}

TEST(JsonHelperTest, SerializesConstVectorToJsonArray)
{
    auto vector = makeVector({4.0, 5.0});
    const PnlVect* const_vector = vector.get();

    nlohmann::json json = const_vector;

    EXPECT_EQ(json, nlohmann::json({4.0, 5.0}));
}

TEST(JsonHelperTest, SerializesVectorToOrderedJsonArray)
{
    auto vector = makeVector({2.0, 1.0});
    nlohmann::ordered_json json;

    to_json(json, vector.get());

    EXPECT_EQ(json, nlohmann::ordered_json({2.0, 1.0}));
}

TEST(JsonHelperTest, DeserializesVectorAndPreservesOrder)
{
    const nlohmann::json json = {3.5, -1.0, 8.25};
    PnlVect* raw_vector = nullptr;

    from_json(json, raw_vector);
    PnlVectPtr vector{raw_vector};

    ASSERT_NE(vector, nullptr);
    ASSERT_EQ(vector->size, 3);
    EXPECT_DOUBLE_EQ(pnl_vect_get(vector.get(), 0), 3.5);
    EXPECT_DOUBLE_EQ(pnl_vect_get(vector.get(), 1), -1.0);
    EXPECT_DOUBLE_EQ(pnl_vect_get(vector.get(), 2), 8.25);
}

TEST(JsonHelperTest, EmptyJsonArrayCreatesEmptyVector)
{
    const nlohmann::json json = nlohmann::json::array();
    PnlVect* raw_vector = nullptr;

    from_json(json, raw_vector);
    PnlVectPtr vector{raw_vector};

    ASSERT_NE(vector, nullptr);
    EXPECT_EQ(vector->size, 0);
}

TEST(JsonHelperTest, InvalidVectorElementTypeThrowsJsonTypeError)
{
    const nlohmann::json json = {1.0, "not a number"};
    PnlVect* vector = nullptr;

    EXPECT_THROW(from_json(json, vector), nlohmann::json::type_error);
    EXPECT_EQ(vector, nullptr);
}

TEST(JsonHelperTest, DeserializesRectangularMatrix)
{
    const nlohmann::json json = {{1.0, 2.0}, {3.0, 4.0}};
    PnlMat* raw_matrix = nullptr;

    from_json(json, raw_matrix);
    PnlMatPtr matrix{raw_matrix};

    ASSERT_NE(matrix, nullptr);
    ASSERT_EQ(matrix->m, 2);
    ASSERT_EQ(matrix->n, 2);
    EXPECT_DOUBLE_EQ(pnl_mat_get(matrix.get(), 0, 0), 1.0);
    EXPECT_DOUBLE_EQ(pnl_mat_get(matrix.get(), 0, 1), 2.0);
    EXPECT_DOUBLE_EQ(pnl_mat_get(matrix.get(), 1, 0), 3.0);
    EXPECT_DOUBLE_EQ(pnl_mat_get(matrix.get(), 1, 1), 4.0);
}

TEST(JsonHelperTest, EmptyJsonArrayCreatesEmptyMatrix)
{
    const nlohmann::json json = nlohmann::json::array();
    PnlMat* raw_matrix = nullptr;

    from_json(json, raw_matrix);
    PnlMatPtr matrix{raw_matrix};

    ASSERT_NE(matrix, nullptr);
    EXPECT_EQ(matrix->m, 0);
    EXPECT_EQ(matrix->n, 0);
}

TEST(JsonHelperTest, RaggedMatrixFailsSafelyWithDiagnostic)
{
    const nlohmann::json json = {{1.0, 2.0}, {3.0}};
    PnlMat* matrix = nullptr;
    testing::internal::CaptureStderr();

    from_json(json, matrix);

    const std::string diagnostic = testing::internal::GetCapturedStderr();
    EXPECT_EQ(matrix, nullptr);
    EXPECT_NE(diagnostic.find("Matrix is not regular"), std::string::npos);
}

TEST(JsonHelperTest, InvalidMatrixElementTypeThrowsJsonTypeError)
{
    const nlohmann::json json = {{1.0, 2.0}, {3.0, "not a number"}};
    PnlMat* matrix = nullptr;

    EXPECT_THROW(from_json(json, matrix), nlohmann::json::type_error);
    EXPECT_EQ(matrix, nullptr);
}

TEST(PricingResultsTest, StreamOutputContainsAllValuesAsJson)
{
    auto delta = makeVector({0.2, 0.8});
    auto delta_std_dev = makeVector({0.01, 0.02});
    PricingResults results{12.5, 0.15, delta.get(), delta_std_dev.get()};
    std::ostringstream output;

    output << results;
    const nlohmann::json json = nlohmann::json::parse(output.str());

    EXPECT_DOUBLE_EQ(json.at("price").get<double>(), 12.5);
    EXPECT_DOUBLE_EQ(json.at("priceStdDev").get<double>(), 0.15);
    EXPECT_EQ(json.at("delta"), nlohmann::json({0.2, 0.8}));
    EXPECT_EQ(json.at("deltaStdDev"), nlohmann::json({0.01, 0.02}));
}
