#include <gtest/gtest.h>
#include "polepp.hpp"
#include "stream_utils.hpp"
#include "test_data.hpp"

std::string getTestFilePath(const char* file_name)
{
    std::string path(TEST_DATA_DIR);
    path += file_name;
    return path;
}

TEST(compound_document, find_storage)
{
    std::string file_path = getTestFilePath("test1.bin");
    ole::compound_document doc(file_path);
    ASSERT_TRUE(doc.good());
    auto begin = doc.begin();
    auto end = doc.end();
    auto storage = doc.find_storage("/Image");
    ASSERT_TRUE(storage != doc.end());
    ASSERT_TRUE(doc.find_storage("/Image/Layers") != doc.end());
    ASSERT_TRUE(doc.find_storage("/Image2") == doc.end());
}

TEST(storage, find_stream)
{
    std::string file_path = getTestFilePath("test1.bin");
    ole::compound_document doc(file_path);
    ASSERT_TRUE(doc.good());
    auto begin = doc.begin();
    auto end = doc.end();
    auto image_storage = doc.find_storage("/Image");
    ASSERT_TRUE(image_storage != doc.end());
    auto content_stream = image_storage->find_stream("/Image/Contents");
    ASSERT_TRUE(content_stream != image_storage->end());
    auto layer_storage = doc.find_storage("/Image/Layers");
    ASSERT_TRUE(layer_storage != doc.end());
    auto stream = layer_storage->find_stream("/Image/Layers");
    ASSERT_TRUE(stream == layer_storage->end());

    auto root_storage = doc.find_storage("/");
    ASSERT_TRUE(root_storage != doc.end());
    auto tag_stream = root_storage->find_stream("/Tags");
    ASSERT_TRUE(tag_stream != root_storage->end());
}


TEST(storage, read_stream_int)
{
    std::string file_path = getTestFilePath("test1.bin");
    ole::compound_document doc(file_path);
    ASSERT_TRUE(doc.good());
    auto begin = doc.begin();
    auto end = doc.end();
    auto storage = doc.find_storage("/Image");
    ASSERT_TRUE(storage != doc.end());
    auto contents = storage->find_stream("/Image/Contents");
    ASSERT_TRUE(contents != storage->end());

    ole::basic_stream stream = contents->stream();
    ole::skipItems(stream, 4);

    int32_t width = ole::readOleInt(stream);
    EXPECT_EQ(width, 1480);

    int32_t height = ole::readOleInt(stream);
    EXPECT_EQ(height, 1132);

    int32_t depth = ole::readOleInt(stream);
    EXPECT_EQ(depth, 0);

    int32_t pixelFormat = ole::readOleInt(stream);
    EXPECT_EQ(pixelFormat, 4);

    int32_t rawCount = ole::readOleInt(stream);
    EXPECT_EQ(rawCount, 3);
}

TEST(storage, read_stream_double)
{
    std::string file_path = getTestFilePath("test1.bin");
    ole::compound_document doc(file_path);
    ASSERT_TRUE(doc.good());
    auto begin = doc.begin();
    auto end = doc.end();
    auto scaling_storage = doc.find_storage("/Image/Scaling");
    ASSERT_TRUE(scaling_storage != doc.end());
    auto contents_stream = scaling_storage->find_stream("/Image/Scaling/Contents");
    ASSERT_TRUE(contents_stream != scaling_storage->end());
    ole::skipItems(contents_stream->stream(), 3);
    double value = ole::readOleDouble(contents_stream->stream());
    ASSERT_DOUBLE_EQ(value, 0.0645);
    int scalingUnits = ole::readOleInt(contents_stream->stream());
    ASSERT_EQ(scalingUnits, 76);
}

TEST(storage, read_stream_string)
{
    std::string file_path = getTestFilePath("test1.bin");
    ole::compound_document doc(file_path);
    ASSERT_TRUE(doc.good());
    auto begin = doc.begin();
    auto end = doc.end();
    auto scaling_storage = doc.find_storage("/Image/Scaling");
    ASSERT_TRUE(scaling_storage != doc.end());
    auto contents_stream = scaling_storage->find_stream("/Image/Scaling/Contents");
    ASSERT_TRUE(contents_stream != scaling_storage->end());
    ole::skipItems(contents_stream->stream(), 1);
    std::string key = ole::readOleString(contents_stream->stream());
    ASSERT_EQ(key, std::string("Scaling124"));
}
