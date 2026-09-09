#include <gtest/gtest.h>
#include <vector>
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

TEST(stream, state_is_clean_on_open)
{
    std::string file_path = getTestFilePath("test1.bin");
    ole::compound_document doc(file_path);
    ASSERT_TRUE(doc.good());
    auto image_storage = doc.find_storage("/Image");
    ASSERT_TRUE(image_storage != doc.end());
    auto content_stream = image_storage->find_stream("/Image/Contents");
    ASSERT_TRUE(content_stream != image_storage->end());
    ole::basic_stream& stream = content_stream->stream();
    // A freshly opened stream has read nothing and failed at nothing.
    // StreamImpl::_state used to be left uninitialised by init(), so both
    // of these read indeterminate memory.
    EXPECT_FALSE(stream.eof());
    EXPECT_FALSE(stream.fail());
}

// The tri-state out-param is what lets the cursor overload skip its flag
// update when the positional read returned without reading anything. Its
// three states are the contract; assert them directly, since the flag
// difference they protect is only observable via the Bad bit, which no valid
// fixture can set.
TEST(stream, positional_read_reports_whether_it_ran)
{
    std::string file_path = getTestFilePath("test1.bin");
    ole::compound_document doc(file_path);
    ASSERT_TRUE(doc.good());
    auto storage = doc.find_storage("/Image");
    ASSERT_TRUE(storage != doc.end());
    auto sp = storage->find_stream("/Image/Contents");
    ASSERT_TRUE(sp != storage->end());
    ole::basic_stream& stream = sp->stream();

    const std::streamoff size = stream.seek(0, std::ios::end);
    stream.seek(0, std::ios::beg);
    ASSERT_GT(size, 8);

    // A zero-length read runs nothing, so no flag update is due.
    std::vector<char> buf((size_t)size);
    EXPECT_EQ(stream.read(buf.data(), 0), 0);
    EXPECT_FALSE(stream.fail());

    // A read that fits reports in-bounds; one that straddles the end clamps.
    stream.seek(0, std::ios::beg);
    EXPECT_EQ(stream.read(buf.data(), 8), 8);
    EXPECT_FALSE(stream.eof());
    stream.seek(size - 4, std::ios::beg);
    EXPECT_EQ(stream.read(buf.data(), 32), 4);
    EXPECT_TRUE(stream.eof());
}
