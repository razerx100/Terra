#include <gtest/gtest.h>
#include <SharedPtrVector.hpp>

TEST(SharedPtrVectorTest, BasicTest)
{
	SharedPtrVector<size_t> sharedVec{};

	auto& vec = sharedVec.GetVector();

	vec.push_back(9);
	vec.push_back(8);
	vec.push_back(2);
	vec.push_back(3);
	vec.push_back(30);
	vec.push_back(7);
	vec.push_back(5);

	EXPECT_EQ(vec.at(4), 30u) << "The Fifth element isn't 30.";

	auto sharedPtr = sharedVec.GetSharedPtr();

	auto sizeTPtr = reinterpret_cast<size_t*>(sharedPtr.get());

	EXPECT_EQ(*(sizeTPtr + 4u), 30u) << "The Fifth element doesn't match.";
}
