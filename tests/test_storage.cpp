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

// Every storage and stream the document enumerates must still be findable by
// the path string the document itself reports -- which is what exercises the
// DirTree path walk that Task 3 makes non-quadratic. Asserting particular
// paths would test the fixture; this asserts the round-trip that makes the
// optimisation safe.
//
// Deliberately does NOT use compound_document::path_exist(): it returns false
// for every nested stream path in this very fixture, because it slices the
// parent path with substr(0, size - ++pos) and lands mid-name. That is a
// pre-existing defect, filed separately, and not something to depend on here.
TEST(dirtree, every_reported_path_resolves)
{
	std::string file_path = getTestFilePath("test1.bin");
	ole::compound_document doc(file_path);
	ASSERT_TRUE(doc.good());

	int storages = 0, streams = 0;
	for (auto it = doc.begin(); it != doc.end(); ++it)
	{
		++storages;
		const std::string storagePath = it->string();
		ASSERT_TRUE(doc.find_storage(storagePath) != doc.end())
			<< "storage does not resolve: " << storagePath;
		for (auto s = it->begin(); s != it->end(); ++s)
		{
			++streams;
			auto owner = doc.find_storage(storagePath);
			ASSERT_TRUE(owner != doc.end());
			EXPECT_TRUE(owner->find_stream(s->string()) != owner->end())
				<< "stream does not resolve: " << s->string();
		}
	}
	EXPECT_EQ(storages, 16);
	EXPECT_EQ(streams, 19);
}

#include <thread>
#include <vector>

// read_at must return exactly what a cursor read returns, and must leave the
// cursor where it found it -- that is what lets one document serve several
// threads without each needing its own copy.
TEST(stream, read_at_matches_cursor_read_and_does_not_move_it)
{
	std::string file_path = getTestFilePath("test1.bin");
	ole::compound_document doc(file_path);
	ASSERT_TRUE(doc.good());
	auto storage = doc.find_storage("/Image");
	ASSERT_TRUE(storage != doc.end());
	auto sp = storage->find_stream("/Image/Contents");
	ASSERT_TRUE(sp != storage->end());
	ole::basic_stream& stream = sp->stream();

	const std::streamoff size = stream.size();
	ASSERT_GT(size, 16);

	std::vector<char> viaCursor(16), viaPositional(16);
	stream.seek(8, std::ios::beg);
	ASSERT_EQ(stream.read(viaCursor.data(), 16), 16);

	stream.seek(0, std::ios::beg);
	ASSERT_EQ(stream.read_at(8, viaPositional.data(), 16), 16);
	EXPECT_EQ(stream.pos(), 0) << "read_at moved the cursor";
	EXPECT_EQ(viaCursor, viaPositional);
}

TEST(stream, read_at_past_end_is_clamped)
{
	std::string file_path = getTestFilePath("test1.bin");
	ole::compound_document doc(file_path);
	ASSERT_TRUE(doc.good());
	auto storage = doc.find_storage("/Image");
	ASSERT_TRUE(storage != doc.end());
	auto sp = storage->find_stream("/Image/Contents");
	ASSERT_TRUE(sp != storage->end());
	ole::basic_stream& stream = sp->stream();

	const std::streamoff size = stream.size();
	std::vector<char> buf(32);
	// Straddling the end returns only what exists, and reports it by count --
	// the destination is otherwise left untouched, so callers must check.
	const std::streamsize got = stream.read_at(size - 4, buf.data(), 32);
	EXPECT_EQ(got, 4);
	EXPECT_EQ(stream.read_at(size, buf.data(), 32), 0);
}

// The race this whole exercise is about: many threads reading one document.
TEST(stream, concurrent_read_at_on_one_document)
{
	std::string file_path = getTestFilePath("test1.bin");
	ole::compound_document doc(file_path);
	ASSERT_TRUE(doc.good());
	auto storage = doc.find_storage("/Image");
	ASSERT_TRUE(storage != doc.end());
	auto sp = storage->find_stream("/Image/Contents");
	ASSERT_TRUE(sp != storage->end());
	const ole::basic_stream& stream = sp->stream();

	const std::streamoff size = stream.size();
	const std::streamsize chunk = (size < 64) ? size : 64;
	ASSERT_GT(chunk, 0);

	std::vector<char> expected((size_t)chunk);
	ASSERT_EQ(stream.read_at(0, expected.data(), chunk), chunk);

	std::vector<std::thread> threads;
	std::vector<int> mismatches(8, 0);
	for (int t = 0; t < 8; ++t)
	{
		threads.emplace_back([&, t]() {
			std::vector<char> got((size_t)chunk);
			for (int i = 0; i < 200; ++i)
			{
				if (stream.read_at(0, got.data(), chunk) != chunk || got != expected)
					++mismatches[t];
			}
		});
	}
	for (auto& th : threads) th.join();
	for (int t = 0; t < 8; ++t)
		EXPECT_EQ(mismatches[t], 0) << "thread " << t << " read torn data";
}
