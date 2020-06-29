/*-------------------------------------------------------------------------
 * Copyright (C) 2020, 4paradigm
 * row.cc
 *
 * Author: chenjing
 * Date: 2020/4/23
 *--------------------------------------------------------------------------
 **/

#include "codec/row.h"

namespace fesql {
namespace codec {

Row::Row() : slice_() {}

Row::Row(const std::string &str)
    : slice_(RefCountedSlice::Create(
          reinterpret_cast<int8_t *>(const_cast<char *>(str.data())),
          str.length())) {}

Row::Row(const Row &s) : slice_(s.slice_), slices_(s.slices_) {}

Row::Row(size_t major_slices, const Row &major, size_t secondary_slices,
         const Row &secondary)
    : slice_(major.slice_), slices_(major_slices + secondary_slices - 1) {
    for (size_t offset = 0; offset < major_slices - 1; ++offset) {
        if (major.slices_.size() > offset) {
            slices_[offset] = major.slices_[offset];
        }
    }
    slices_[major_slices - 1] = secondary.slice_;
    for (size_t offset = 0; offset < secondary_slices - 1; ++offset) {
        if (secondary.slices_.size() > offset) {
            slices_[offset + major_slices] = secondary.slices_[offset];
        }
    }
}

Row::Row(const RefCountedSlice &s) : slice_(s) {}

Row::~Row() {}

void Row::Append(const std::vector<RefCountedSlice> &slices) {
    if (!slices.empty()) {
        for (auto iter = slices.cbegin(); iter != slices.cend(); iter++) {
            slices_.push_back(*iter);
        }
    }
}
void Row::Append(const Row &b) {
    slices_.push_back(b.slice_);
    Append(b.slices_);
}

int32_t Row::GetRowPtrCnt() const { return 1 + slices_.size(); }

// Return a string that contains the copy of the referenced data.
std::string Row::ToString() const { return slice_.ToString(); }

int Row::compare(const Row &b) const {
    int r = slice_.compare(b.slice_);
    if (r != 0) {
        return r;
    }
    size_t this_len = slices_.size();
    size_t b_len = b.slices_.size();
    size_t min_len = this_len < b_len ? this_len : b_len;
    for (size_t i = 0; i < min_len; i++) {
        int slice_compared = slices_[i].compare(b.slices_[i]);
        if (0 == slice_compared) {
            continue;
        }
        return slice_compared;
    }

    return this_len < b_len ? -1 : this_len > b_len ? +1 : 0;
}

int8_t **Row::GetRowPtrs() const {
    if (slices_.empty()) {
        return new int8_t *[1] { slice_.buf() };
    } else {
        int8_t **ptrs = new int8_t *[slices_.size() + 1];
        int pos = 0;
        ptrs[pos++] = slice_.buf();
        for (auto slice : slices_) {
            ptrs[pos++] = slice.buf();
        }
        return ptrs;
    }
}
int32_t *Row::GetRowSizes() const {
    if (slices_.empty()) {
        return new int32_t[1]{static_cast<int32_t>(slice_.size())};
    } else {
        int32_t *sizes = new int32_t[slices_.size() + 1];
        int pos = 0;
        sizes[pos++] = slice_.size();
        for (auto slice : slices_) {
            sizes[pos++] = static_cast<int32_t>(slice.size());
        }
        return sizes;
    }
}

}  // namespace codec
}  // namespace fesql
