#pragma once

#include "baseentry.hpp"
#include "package.hpp"
#include <stack>
#include <deque>

namespace ShadyCore {
	class ZipStream : public std::streambuf {
	private:
		void *pkgFile = 0;
		void *innerFile = 0;
		unsigned int index;
		std::stack<int_type> pool;
		std::streamoff pos = 0;
		std::streamsize size;
		int_type bufVal;
		bool hasBufVal = false;

	protected:
		// DISALLOWED
		int_type pbackfail(int_type) override { throw; }
		void imbue(const std::locale&) override { throw; }
		std::streambuf* setbuf(char_type*, std::streamsize) override { throw; }
		std::streamsize xsputn(const char_type*, std::streamsize) override { throw; }
		int_type overflow(int_type value) override { throw; }

		pos_type seekoff(off_type, std::ios::seekdir, std::ios::openmode) override;
		pos_type seekpos(pos_type, std::ios::openmode) override;
		std::streamsize showmanyc() override { return size - pos; }

		std::streamsize xsgetn(char_type*, std::streamsize) override;
		int_type underflow() override;
		int_type uflow() override;
	public:
		inline bool isOpen() const { return pkgFile; }
		void open(const std::filesystem::path&, const std::string&);
		void close();
	};

	class ZipPackageEntry : public BasePackageEntry {
	private:
		const std::string name;

		std::deque<ZipStream> zipBuffer;
		std::deque<std::istream> zipStream;
	public:
		inline ZipPackageEntry(Package* parent, const std::string& name, unsigned int size)
			: BasePackageEntry(parent, size), name(name) {}

		inline StorageType getStorage() const override final { return TYPE_ZIP; }
		inline bool isOpen() const override final { return zipBuffer.size(); }
		inline std::istream& open() override final {
			zipBuffer.emplace_back();
			zipStream.emplace_back(&zipBuffer.back());
			zipBuffer.back().open(parent->getBasePath(), name);
			return zipStream.back();
		}
		inline void close() override final { zipBuffer.front().close(); zipBuffer.pop_front(); zipStream.pop_front(); if (disposable && !zipBuffer.size()) delete this; }
	};

	ShadyCore::FileType GetZipPackageDefaultType(const ShadyCore::FileType& inputType, ShadyCore::BasePackageEntry* entry);
}
