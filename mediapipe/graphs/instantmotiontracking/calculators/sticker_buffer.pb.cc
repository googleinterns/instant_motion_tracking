// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: sticker_buffer.proto

#include "sticker_buffer.pb.h"

#include <algorithm>

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/extension_set.h>
#include <google/protobuf/wire_format_lite.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/generated_message_reflection.h>
#include <google/protobuf/reflection_ops.h>
#include <google/protobuf/wire_format.h>
// @@protoc_insertion_point(includes)
#include <google/protobuf/port_def.inc>
extern PROTOBUF_INTERNAL_EXPORT_sticker_5fbuffer_2eproto ::PROTOBUF_NAMESPACE_ID::internal::SCCInfo<0> scc_info_Sticker_sticker_5fbuffer_2eproto;
namespace instantmotiontracking {
class StickerDefaultTypeInternal {
 public:
  ::PROTOBUF_NAMESPACE_ID::internal::ExplicitlyConstructed<Sticker> _instance;
} _Sticker_default_instance_;
class StickerRollDefaultTypeInternal {
 public:
  ::PROTOBUF_NAMESPACE_ID::internal::ExplicitlyConstructed<StickerRoll> _instance;
} _StickerRoll_default_instance_;
}  // namespace instantmotiontracking
static void InitDefaultsscc_info_Sticker_sticker_5fbuffer_2eproto() {
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  {
    void* ptr = &::instantmotiontracking::_Sticker_default_instance_;
    new (ptr) ::instantmotiontracking::Sticker();
    ::PROTOBUF_NAMESPACE_ID::internal::OnShutdownDestroyMessage(ptr);
  }
  ::instantmotiontracking::Sticker::InitAsDefaultInstance();
}

::PROTOBUF_NAMESPACE_ID::internal::SCCInfo<0> scc_info_Sticker_sticker_5fbuffer_2eproto =
    {{ATOMIC_VAR_INIT(::PROTOBUF_NAMESPACE_ID::internal::SCCInfoBase::kUninitialized), 0, 0, InitDefaultsscc_info_Sticker_sticker_5fbuffer_2eproto}, {}};

static void InitDefaultsscc_info_StickerRoll_sticker_5fbuffer_2eproto() {
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  {
    void* ptr = &::instantmotiontracking::_StickerRoll_default_instance_;
    new (ptr) ::instantmotiontracking::StickerRoll();
    ::PROTOBUF_NAMESPACE_ID::internal::OnShutdownDestroyMessage(ptr);
  }
  ::instantmotiontracking::StickerRoll::InitAsDefaultInstance();
}

::PROTOBUF_NAMESPACE_ID::internal::SCCInfo<1> scc_info_StickerRoll_sticker_5fbuffer_2eproto =
    {{ATOMIC_VAR_INIT(::PROTOBUF_NAMESPACE_ID::internal::SCCInfoBase::kUninitialized), 1, 0, InitDefaultsscc_info_StickerRoll_sticker_5fbuffer_2eproto}, {
      &scc_info_Sticker_sticker_5fbuffer_2eproto.base,}};

static ::PROTOBUF_NAMESPACE_ID::Metadata file_level_metadata_sticker_5fbuffer_2eproto[2];
static constexpr ::PROTOBUF_NAMESPACE_ID::EnumDescriptor const** file_level_enum_descriptors_sticker_5fbuffer_2eproto = nullptr;
static constexpr ::PROTOBUF_NAMESPACE_ID::ServiceDescriptor const** file_level_service_descriptors_sticker_5fbuffer_2eproto = nullptr;

const ::PROTOBUF_NAMESPACE_ID::uint32 TableStruct_sticker_5fbuffer_2eproto::offsets[] PROTOBUF_SECTION_VARIABLE(protodesc_cold) = {
  PROTOBUF_FIELD_OFFSET(::instantmotiontracking::Sticker, _has_bits_),
  PROTOBUF_FIELD_OFFSET(::instantmotiontracking::Sticker, _internal_metadata_),
  ~0u,  // no _extensions_
  ~0u,  // no _oneof_case_
  ~0u,  // no _weak_field_map_
  PROTOBUF_FIELD_OFFSET(::instantmotiontracking::Sticker, id_),
  PROTOBUF_FIELD_OFFSET(::instantmotiontracking::Sticker, x_),
  PROTOBUF_FIELD_OFFSET(::instantmotiontracking::Sticker, y_),
  PROTOBUF_FIELD_OFFSET(::instantmotiontracking::Sticker, rotation_),
  PROTOBUF_FIELD_OFFSET(::instantmotiontracking::Sticker, scale_),
  PROTOBUF_FIELD_OFFSET(::instantmotiontracking::Sticker, renderid_),
  0,
  1,
  2,
  3,
  4,
  5,
  PROTOBUF_FIELD_OFFSET(::instantmotiontracking::StickerRoll, _has_bits_),
  PROTOBUF_FIELD_OFFSET(::instantmotiontracking::StickerRoll, _internal_metadata_),
  ~0u,  // no _extensions_
  ~0u,  // no _oneof_case_
  ~0u,  // no _weak_field_map_
  PROTOBUF_FIELD_OFFSET(::instantmotiontracking::StickerRoll, sticker_),
  ~0u,
};
static const ::PROTOBUF_NAMESPACE_ID::internal::MigrationSchema schemas[] PROTOBUF_SECTION_VARIABLE(protodesc_cold) = {
  { 0, 11, sizeof(::instantmotiontracking::Sticker)},
  { 17, 23, sizeof(::instantmotiontracking::StickerRoll)},
};

static ::PROTOBUF_NAMESPACE_ID::Message const * const file_default_instances[] = {
  reinterpret_cast<const ::PROTOBUF_NAMESPACE_ID::Message*>(&::instantmotiontracking::_Sticker_default_instance_),
  reinterpret_cast<const ::PROTOBUF_NAMESPACE_ID::Message*>(&::instantmotiontracking::_StickerRoll_default_instance_),
};

const char descriptor_table_protodef_sticker_5fbuffer_2eproto[] PROTOBUF_SECTION_VARIABLE(protodesc_cold) =
  "\n\024sticker_buffer.proto\022\025instantmotiontra"
  "cking\"^\n\007Sticker\022\n\n\002id\030\001 \002(\005\022\t\n\001x\030\002 \002(\002\022"
  "\t\n\001y\030\003 \002(\002\022\020\n\010rotation\030\004 \002(\002\022\r\n\005scale\030\005 "
  "\002(\002\022\020\n\010renderID\030\006 \002(\005\">\n\013StickerRoll\022/\n\007"
  "sticker\030\001 \003(\0132\036.instantmotiontracking.St"
  "ickerB1\n/com.google.mediapipe.apps.insta"
  "ntmotiontracking"
  ;
static const ::PROTOBUF_NAMESPACE_ID::internal::DescriptorTable*const descriptor_table_sticker_5fbuffer_2eproto_deps[1] = {
};
static ::PROTOBUF_NAMESPACE_ID::internal::SCCInfoBase*const descriptor_table_sticker_5fbuffer_2eproto_sccs[2] = {
  &scc_info_Sticker_sticker_5fbuffer_2eproto.base,
  &scc_info_StickerRoll_sticker_5fbuffer_2eproto.base,
};
static ::PROTOBUF_NAMESPACE_ID::internal::once_flag descriptor_table_sticker_5fbuffer_2eproto_once;
static bool descriptor_table_sticker_5fbuffer_2eproto_initialized = false;
const ::PROTOBUF_NAMESPACE_ID::internal::DescriptorTable descriptor_table_sticker_5fbuffer_2eproto = {
  &descriptor_table_sticker_5fbuffer_2eproto_initialized, descriptor_table_protodef_sticker_5fbuffer_2eproto, "sticker_buffer.proto", 256,
  &descriptor_table_sticker_5fbuffer_2eproto_once, descriptor_table_sticker_5fbuffer_2eproto_sccs, descriptor_table_sticker_5fbuffer_2eproto_deps, 2, 0,
  schemas, file_default_instances, TableStruct_sticker_5fbuffer_2eproto::offsets,
  file_level_metadata_sticker_5fbuffer_2eproto, 2, file_level_enum_descriptors_sticker_5fbuffer_2eproto, file_level_service_descriptors_sticker_5fbuffer_2eproto,
};

// Force running AddDescriptors() at dynamic initialization time.
static bool dynamic_init_dummy_sticker_5fbuffer_2eproto = (  ::PROTOBUF_NAMESPACE_ID::internal::AddDescriptors(&descriptor_table_sticker_5fbuffer_2eproto), true);
namespace instantmotiontracking {

// ===================================================================

void Sticker::InitAsDefaultInstance() {
}
class Sticker::_Internal {
 public:
  using HasBits = decltype(std::declval<Sticker>()._has_bits_);
  static void set_has_id(HasBits* has_bits) {
    (*has_bits)[0] |= 1u;
  }
  static void set_has_x(HasBits* has_bits) {
    (*has_bits)[0] |= 2u;
  }
  static void set_has_y(HasBits* has_bits) {
    (*has_bits)[0] |= 4u;
  }
  static void set_has_rotation(HasBits* has_bits) {
    (*has_bits)[0] |= 8u;
  }
  static void set_has_scale(HasBits* has_bits) {
    (*has_bits)[0] |= 16u;
  }
  static void set_has_renderid(HasBits* has_bits) {
    (*has_bits)[0] |= 32u;
  }
};

Sticker::Sticker()
  : ::PROTOBUF_NAMESPACE_ID::Message(), _internal_metadata_(nullptr) {
  SharedCtor();
  // @@protoc_insertion_point(constructor:instantmotiontracking.Sticker)
}
Sticker::Sticker(const Sticker& from)
  : ::PROTOBUF_NAMESPACE_ID::Message(),
      _internal_metadata_(nullptr),
      _has_bits_(from._has_bits_) {
  _internal_metadata_.MergeFrom(from._internal_metadata_);
  ::memcpy(&id_, &from.id_,
    static_cast<size_t>(reinterpret_cast<char*>(&renderid_) -
    reinterpret_cast<char*>(&id_)) + sizeof(renderid_));
  // @@protoc_insertion_point(copy_constructor:instantmotiontracking.Sticker)
}

void Sticker::SharedCtor() {
  ::memset(&id_, 0, static_cast<size_t>(
      reinterpret_cast<char*>(&renderid_) -
      reinterpret_cast<char*>(&id_)) + sizeof(renderid_));
}

Sticker::~Sticker() {
  // @@protoc_insertion_point(destructor:instantmotiontracking.Sticker)
  SharedDtor();
}

void Sticker::SharedDtor() {
}

void Sticker::SetCachedSize(int size) const {
  _cached_size_.Set(size);
}
const Sticker& Sticker::default_instance() {
  ::PROTOBUF_NAMESPACE_ID::internal::InitSCC(&::scc_info_Sticker_sticker_5fbuffer_2eproto.base);
  return *internal_default_instance();
}


void Sticker::Clear() {
// @@protoc_insertion_point(message_clear_start:instantmotiontracking.Sticker)
  ::PROTOBUF_NAMESPACE_ID::uint32 cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  cached_has_bits = _has_bits_[0];
  if (cached_has_bits & 0x0000003fu) {
    ::memset(&id_, 0, static_cast<size_t>(
        reinterpret_cast<char*>(&renderid_) -
        reinterpret_cast<char*>(&id_)) + sizeof(renderid_));
  }
  _has_bits_.Clear();
  _internal_metadata_.Clear();
}

const char* Sticker::_InternalParse(const char* ptr, ::PROTOBUF_NAMESPACE_ID::internal::ParseContext* ctx) {
#define CHK_(x) if (PROTOBUF_PREDICT_FALSE(!(x))) goto failure
  _Internal::HasBits has_bits{};
  while (!ctx->Done(&ptr)) {
    ::PROTOBUF_NAMESPACE_ID::uint32 tag;
    ptr = ::PROTOBUF_NAMESPACE_ID::internal::ReadTag(ptr, &tag);
    CHK_(ptr);
    switch (tag >> 3) {
      // required int32 id = 1;
      case 1:
        if (PROTOBUF_PREDICT_TRUE(static_cast<::PROTOBUF_NAMESPACE_ID::uint8>(tag) == 8)) {
          _Internal::set_has_id(&has_bits);
          id_ = ::PROTOBUF_NAMESPACE_ID::internal::ReadVarint(&ptr);
          CHK_(ptr);
        } else goto handle_unusual;
        continue;
      // required float x = 2;
      case 2:
        if (PROTOBUF_PREDICT_TRUE(static_cast<::PROTOBUF_NAMESPACE_ID::uint8>(tag) == 21)) {
          _Internal::set_has_x(&has_bits);
          x_ = ::PROTOBUF_NAMESPACE_ID::internal::UnalignedLoad<float>(ptr);
          ptr += sizeof(float);
        } else goto handle_unusual;
        continue;
      // required float y = 3;
      case 3:
        if (PROTOBUF_PREDICT_TRUE(static_cast<::PROTOBUF_NAMESPACE_ID::uint8>(tag) == 29)) {
          _Internal::set_has_y(&has_bits);
          y_ = ::PROTOBUF_NAMESPACE_ID::internal::UnalignedLoad<float>(ptr);
          ptr += sizeof(float);
        } else goto handle_unusual;
        continue;
      // required float rotation = 4;
      case 4:
        if (PROTOBUF_PREDICT_TRUE(static_cast<::PROTOBUF_NAMESPACE_ID::uint8>(tag) == 37)) {
          _Internal::set_has_rotation(&has_bits);
          rotation_ = ::PROTOBUF_NAMESPACE_ID::internal::UnalignedLoad<float>(ptr);
          ptr += sizeof(float);
        } else goto handle_unusual;
        continue;
      // required float scale = 5;
      case 5:
        if (PROTOBUF_PREDICT_TRUE(static_cast<::PROTOBUF_NAMESPACE_ID::uint8>(tag) == 45)) {
          _Internal::set_has_scale(&has_bits);
          scale_ = ::PROTOBUF_NAMESPACE_ID::internal::UnalignedLoad<float>(ptr);
          ptr += sizeof(float);
        } else goto handle_unusual;
        continue;
      // required int32 renderID = 6;
      case 6:
        if (PROTOBUF_PREDICT_TRUE(static_cast<::PROTOBUF_NAMESPACE_ID::uint8>(tag) == 48)) {
          _Internal::set_has_renderid(&has_bits);
          renderid_ = ::PROTOBUF_NAMESPACE_ID::internal::ReadVarint(&ptr);
          CHK_(ptr);
        } else goto handle_unusual;
        continue;
      default: {
      handle_unusual:
        if ((tag & 7) == 4 || tag == 0) {
          ctx->SetLastTag(tag);
          goto success;
        }
        ptr = UnknownFieldParse(tag, &_internal_metadata_, ptr, ctx);
        CHK_(ptr != nullptr);
        continue;
      }
    }  // switch
  }  // while
success:
  _has_bits_.Or(has_bits);
  return ptr;
failure:
  ptr = nullptr;
  goto success;
#undef CHK_
}

::PROTOBUF_NAMESPACE_ID::uint8* Sticker::_InternalSerialize(
    ::PROTOBUF_NAMESPACE_ID::uint8* target, ::PROTOBUF_NAMESPACE_ID::io::EpsCopyOutputStream* stream) const {
  // @@protoc_insertion_point(serialize_to_array_start:instantmotiontracking.Sticker)
  ::PROTOBUF_NAMESPACE_ID::uint32 cached_has_bits = 0;
  (void) cached_has_bits;

  cached_has_bits = _has_bits_[0];
  // required int32 id = 1;
  if (cached_has_bits & 0x00000001u) {
    target = stream->EnsureSpace(target);
    target = ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::WriteInt32ToArray(1, this->_internal_id(), target);
  }

  // required float x = 2;
  if (cached_has_bits & 0x00000002u) {
    target = stream->EnsureSpace(target);
    target = ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::WriteFloatToArray(2, this->_internal_x(), target);
  }

  // required float y = 3;
  if (cached_has_bits & 0x00000004u) {
    target = stream->EnsureSpace(target);
    target = ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::WriteFloatToArray(3, this->_internal_y(), target);
  }

  // required float rotation = 4;
  if (cached_has_bits & 0x00000008u) {
    target = stream->EnsureSpace(target);
    target = ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::WriteFloatToArray(4, this->_internal_rotation(), target);
  }

  // required float scale = 5;
  if (cached_has_bits & 0x00000010u) {
    target = stream->EnsureSpace(target);
    target = ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::WriteFloatToArray(5, this->_internal_scale(), target);
  }

  // required int32 renderID = 6;
  if (cached_has_bits & 0x00000020u) {
    target = stream->EnsureSpace(target);
    target = ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::WriteInt32ToArray(6, this->_internal_renderid(), target);
  }

  if (PROTOBUF_PREDICT_FALSE(_internal_metadata_.have_unknown_fields())) {
    target = ::PROTOBUF_NAMESPACE_ID::internal::WireFormat::InternalSerializeUnknownFieldsToArray(
        _internal_metadata_.unknown_fields(), target, stream);
  }
  // @@protoc_insertion_point(serialize_to_array_end:instantmotiontracking.Sticker)
  return target;
}

size_t Sticker::RequiredFieldsByteSizeFallback() const {
// @@protoc_insertion_point(required_fields_byte_size_fallback_start:instantmotiontracking.Sticker)
  size_t total_size = 0;

  if (_internal_has_id()) {
    // required int32 id = 1;
    total_size += 1 +
      ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::Int32Size(
        this->_internal_id());
  }

  if (_internal_has_x()) {
    // required float x = 2;
    total_size += 1 + 4;
  }

  if (_internal_has_y()) {
    // required float y = 3;
    total_size += 1 + 4;
  }

  if (_internal_has_rotation()) {
    // required float rotation = 4;
    total_size += 1 + 4;
  }

  if (_internal_has_scale()) {
    // required float scale = 5;
    total_size += 1 + 4;
  }

  if (_internal_has_renderid()) {
    // required int32 renderID = 6;
    total_size += 1 +
      ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::Int32Size(
        this->_internal_renderid());
  }

  return total_size;
}
size_t Sticker::ByteSizeLong() const {
// @@protoc_insertion_point(message_byte_size_start:instantmotiontracking.Sticker)
  size_t total_size = 0;

  if (((_has_bits_[0] & 0x0000003f) ^ 0x0000003f) == 0) {  // All required fields are present.
    // required int32 id = 1;
    total_size += 1 +
      ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::Int32Size(
        this->_internal_id());

    // required float x = 2;
    total_size += 1 + 4;

    // required float y = 3;
    total_size += 1 + 4;

    // required float rotation = 4;
    total_size += 1 + 4;

    // required float scale = 5;
    total_size += 1 + 4;

    // required int32 renderID = 6;
    total_size += 1 +
      ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::Int32Size(
        this->_internal_renderid());

  } else {
    total_size += RequiredFieldsByteSizeFallback();
  }
  ::PROTOBUF_NAMESPACE_ID::uint32 cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  if (PROTOBUF_PREDICT_FALSE(_internal_metadata_.have_unknown_fields())) {
    return ::PROTOBUF_NAMESPACE_ID::internal::ComputeUnknownFieldsSize(
        _internal_metadata_, total_size, &_cached_size_);
  }
  int cached_size = ::PROTOBUF_NAMESPACE_ID::internal::ToCachedSize(total_size);
  SetCachedSize(cached_size);
  return total_size;
}

void Sticker::MergeFrom(const ::PROTOBUF_NAMESPACE_ID::Message& from) {
// @@protoc_insertion_point(generalized_merge_from_start:instantmotiontracking.Sticker)
  GOOGLE_DCHECK_NE(&from, this);
  const Sticker* source =
      ::PROTOBUF_NAMESPACE_ID::DynamicCastToGenerated<Sticker>(
          &from);
  if (source == nullptr) {
  // @@protoc_insertion_point(generalized_merge_from_cast_fail:instantmotiontracking.Sticker)
    ::PROTOBUF_NAMESPACE_ID::internal::ReflectionOps::Merge(from, this);
  } else {
  // @@protoc_insertion_point(generalized_merge_from_cast_success:instantmotiontracking.Sticker)
    MergeFrom(*source);
  }
}

void Sticker::MergeFrom(const Sticker& from) {
// @@protoc_insertion_point(class_specific_merge_from_start:instantmotiontracking.Sticker)
  GOOGLE_DCHECK_NE(&from, this);
  _internal_metadata_.MergeFrom(from._internal_metadata_);
  ::PROTOBUF_NAMESPACE_ID::uint32 cached_has_bits = 0;
  (void) cached_has_bits;

  cached_has_bits = from._has_bits_[0];
  if (cached_has_bits & 0x0000003fu) {
    if (cached_has_bits & 0x00000001u) {
      id_ = from.id_;
    }
    if (cached_has_bits & 0x00000002u) {
      x_ = from.x_;
    }
    if (cached_has_bits & 0x00000004u) {
      y_ = from.y_;
    }
    if (cached_has_bits & 0x00000008u) {
      rotation_ = from.rotation_;
    }
    if (cached_has_bits & 0x00000010u) {
      scale_ = from.scale_;
    }
    if (cached_has_bits & 0x00000020u) {
      renderid_ = from.renderid_;
    }
    _has_bits_[0] |= cached_has_bits;
  }
}

void Sticker::CopyFrom(const ::PROTOBUF_NAMESPACE_ID::Message& from) {
// @@protoc_insertion_point(generalized_copy_from_start:instantmotiontracking.Sticker)
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

void Sticker::CopyFrom(const Sticker& from) {
// @@protoc_insertion_point(class_specific_copy_from_start:instantmotiontracking.Sticker)
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool Sticker::IsInitialized() const {
  if ((_has_bits_[0] & 0x0000003f) != 0x0000003f) return false;
  return true;
}

void Sticker::InternalSwap(Sticker* other) {
  using std::swap;
  _internal_metadata_.Swap(&other->_internal_metadata_);
  swap(_has_bits_[0], other->_has_bits_[0]);
  swap(id_, other->id_);
  swap(x_, other->x_);
  swap(y_, other->y_);
  swap(rotation_, other->rotation_);
  swap(scale_, other->scale_);
  swap(renderid_, other->renderid_);
}

::PROTOBUF_NAMESPACE_ID::Metadata Sticker::GetMetadata() const {
  return GetMetadataStatic();
}


// ===================================================================

void StickerRoll::InitAsDefaultInstance() {
}
class StickerRoll::_Internal {
 public:
  using HasBits = decltype(std::declval<StickerRoll>()._has_bits_);
};

StickerRoll::StickerRoll()
  : ::PROTOBUF_NAMESPACE_ID::Message(), _internal_metadata_(nullptr) {
  SharedCtor();
  // @@protoc_insertion_point(constructor:instantmotiontracking.StickerRoll)
}
StickerRoll::StickerRoll(const StickerRoll& from)
  : ::PROTOBUF_NAMESPACE_ID::Message(),
      _internal_metadata_(nullptr),
      _has_bits_(from._has_bits_),
      sticker_(from.sticker_) {
  _internal_metadata_.MergeFrom(from._internal_metadata_);
  // @@protoc_insertion_point(copy_constructor:instantmotiontracking.StickerRoll)
}

void StickerRoll::SharedCtor() {
  ::PROTOBUF_NAMESPACE_ID::internal::InitSCC(&scc_info_StickerRoll_sticker_5fbuffer_2eproto.base);
}

StickerRoll::~StickerRoll() {
  // @@protoc_insertion_point(destructor:instantmotiontracking.StickerRoll)
  SharedDtor();
}

void StickerRoll::SharedDtor() {
}

void StickerRoll::SetCachedSize(int size) const {
  _cached_size_.Set(size);
}
const StickerRoll& StickerRoll::default_instance() {
  ::PROTOBUF_NAMESPACE_ID::internal::InitSCC(&::scc_info_StickerRoll_sticker_5fbuffer_2eproto.base);
  return *internal_default_instance();
}


void StickerRoll::Clear() {
// @@protoc_insertion_point(message_clear_start:instantmotiontracking.StickerRoll)
  ::PROTOBUF_NAMESPACE_ID::uint32 cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  sticker_.Clear();
  _has_bits_.Clear();
  _internal_metadata_.Clear();
}

const char* StickerRoll::_InternalParse(const char* ptr, ::PROTOBUF_NAMESPACE_ID::internal::ParseContext* ctx) {
#define CHK_(x) if (PROTOBUF_PREDICT_FALSE(!(x))) goto failure
  while (!ctx->Done(&ptr)) {
    ::PROTOBUF_NAMESPACE_ID::uint32 tag;
    ptr = ::PROTOBUF_NAMESPACE_ID::internal::ReadTag(ptr, &tag);
    CHK_(ptr);
    switch (tag >> 3) {
      // repeated .instantmotiontracking.Sticker sticker = 1;
      case 1:
        if (PROTOBUF_PREDICT_TRUE(static_cast<::PROTOBUF_NAMESPACE_ID::uint8>(tag) == 10)) {
          ptr -= 1;
          do {
            ptr += 1;
            ptr = ctx->ParseMessage(_internal_add_sticker(), ptr);
            CHK_(ptr);
            if (!ctx->DataAvailable(ptr)) break;
          } while (::PROTOBUF_NAMESPACE_ID::internal::ExpectTag<10>(ptr));
        } else goto handle_unusual;
        continue;
      default: {
      handle_unusual:
        if ((tag & 7) == 4 || tag == 0) {
          ctx->SetLastTag(tag);
          goto success;
        }
        ptr = UnknownFieldParse(tag, &_internal_metadata_, ptr, ctx);
        CHK_(ptr != nullptr);
        continue;
      }
    }  // switch
  }  // while
success:
  return ptr;
failure:
  ptr = nullptr;
  goto success;
#undef CHK_
}

::PROTOBUF_NAMESPACE_ID::uint8* StickerRoll::_InternalSerialize(
    ::PROTOBUF_NAMESPACE_ID::uint8* target, ::PROTOBUF_NAMESPACE_ID::io::EpsCopyOutputStream* stream) const {
  // @@protoc_insertion_point(serialize_to_array_start:instantmotiontracking.StickerRoll)
  ::PROTOBUF_NAMESPACE_ID::uint32 cached_has_bits = 0;
  (void) cached_has_bits;

  // repeated .instantmotiontracking.Sticker sticker = 1;
  for (unsigned int i = 0,
      n = static_cast<unsigned int>(this->_internal_sticker_size()); i < n; i++) {
    target = stream->EnsureSpace(target);
    target = ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::
      InternalWriteMessage(1, this->_internal_sticker(i), target, stream);
  }

  if (PROTOBUF_PREDICT_FALSE(_internal_metadata_.have_unknown_fields())) {
    target = ::PROTOBUF_NAMESPACE_ID::internal::WireFormat::InternalSerializeUnknownFieldsToArray(
        _internal_metadata_.unknown_fields(), target, stream);
  }
  // @@protoc_insertion_point(serialize_to_array_end:instantmotiontracking.StickerRoll)
  return target;
}

size_t StickerRoll::ByteSizeLong() const {
// @@protoc_insertion_point(message_byte_size_start:instantmotiontracking.StickerRoll)
  size_t total_size = 0;

  ::PROTOBUF_NAMESPACE_ID::uint32 cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  // repeated .instantmotiontracking.Sticker sticker = 1;
  total_size += 1UL * this->_internal_sticker_size();
  for (const auto& msg : this->sticker_) {
    total_size +=
      ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::MessageSize(msg);
  }

  if (PROTOBUF_PREDICT_FALSE(_internal_metadata_.have_unknown_fields())) {
    return ::PROTOBUF_NAMESPACE_ID::internal::ComputeUnknownFieldsSize(
        _internal_metadata_, total_size, &_cached_size_);
  }
  int cached_size = ::PROTOBUF_NAMESPACE_ID::internal::ToCachedSize(total_size);
  SetCachedSize(cached_size);
  return total_size;
}

void StickerRoll::MergeFrom(const ::PROTOBUF_NAMESPACE_ID::Message& from) {
// @@protoc_insertion_point(generalized_merge_from_start:instantmotiontracking.StickerRoll)
  GOOGLE_DCHECK_NE(&from, this);
  const StickerRoll* source =
      ::PROTOBUF_NAMESPACE_ID::DynamicCastToGenerated<StickerRoll>(
          &from);
  if (source == nullptr) {
  // @@protoc_insertion_point(generalized_merge_from_cast_fail:instantmotiontracking.StickerRoll)
    ::PROTOBUF_NAMESPACE_ID::internal::ReflectionOps::Merge(from, this);
  } else {
  // @@protoc_insertion_point(generalized_merge_from_cast_success:instantmotiontracking.StickerRoll)
    MergeFrom(*source);
  }
}

void StickerRoll::MergeFrom(const StickerRoll& from) {
// @@protoc_insertion_point(class_specific_merge_from_start:instantmotiontracking.StickerRoll)
  GOOGLE_DCHECK_NE(&from, this);
  _internal_metadata_.MergeFrom(from._internal_metadata_);
  ::PROTOBUF_NAMESPACE_ID::uint32 cached_has_bits = 0;
  (void) cached_has_bits;

  sticker_.MergeFrom(from.sticker_);
}

void StickerRoll::CopyFrom(const ::PROTOBUF_NAMESPACE_ID::Message& from) {
// @@protoc_insertion_point(generalized_copy_from_start:instantmotiontracking.StickerRoll)
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

void StickerRoll::CopyFrom(const StickerRoll& from) {
// @@protoc_insertion_point(class_specific_copy_from_start:instantmotiontracking.StickerRoll)
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool StickerRoll::IsInitialized() const {
  if (!::PROTOBUF_NAMESPACE_ID::internal::AllAreInitialized(sticker_)) return false;
  return true;
}

void StickerRoll::InternalSwap(StickerRoll* other) {
  using std::swap;
  _internal_metadata_.Swap(&other->_internal_metadata_);
  swap(_has_bits_[0], other->_has_bits_[0]);
  sticker_.InternalSwap(&other->sticker_);
}

::PROTOBUF_NAMESPACE_ID::Metadata StickerRoll::GetMetadata() const {
  return GetMetadataStatic();
}


// @@protoc_insertion_point(namespace_scope)
}  // namespace instantmotiontracking
PROTOBUF_NAMESPACE_OPEN
template<> PROTOBUF_NOINLINE ::instantmotiontracking::Sticker* Arena::CreateMaybeMessage< ::instantmotiontracking::Sticker >(Arena* arena) {
  return Arena::CreateInternal< ::instantmotiontracking::Sticker >(arena);
}
template<> PROTOBUF_NOINLINE ::instantmotiontracking::StickerRoll* Arena::CreateMaybeMessage< ::instantmotiontracking::StickerRoll >(Arena* arena) {
  return Arena::CreateInternal< ::instantmotiontracking::StickerRoll >(arena);
}
PROTOBUF_NAMESPACE_CLOSE

// @@protoc_insertion_point(global_scope)
#include <google/protobuf/port_undef.inc>
