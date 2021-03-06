#ifndef _A4_DYNAMIC_MESSAGE_H_
#define _A4_DYNAMIC_MESSAGE_H_

#include <unordered_set>
#include <string>

#include <a4/types.h>
#include <a4/string.h>

#include <boost/variant.hpp>
#include <boost/variant/get.hpp>

#include <google/protobuf/message.h>
#include <google/protobuf/descriptor.h>

#include <a4/io/A4.pb.h>

using google::protobuf::Message;
using google::protobuf::FieldDescriptor;
using google::protobuf::EnumValueDescriptor;
using google::protobuf::Reflection;

namespace a4 {
namespace io {

typedef boost::variant<int32_t, int64_t, uint32_t, uint64_t, double, float, bool, std::string> FieldContentVariant;

struct variant_adder : public boost::static_visitor<FieldContentVariant> {
    template <typename T>
    FieldContentVariant operator()(const T& i, const T& j) const { return i + j; };
    template <typename T, typename U>
    FieldContentVariant operator()( const T &, const U& ) const {return false;}
};

struct variant_multiplier : public boost::static_visitor<FieldContentVariant> {
    FieldContentVariant operator()(const std::string& i, const std::string& j) const { return i + j; };
    FieldContentVariant operator()(const bool& i, const bool& j) const { return i && j; };
    template <typename T>
    FieldContentVariant operator()(const T& i, const T& j) const { return i * j; };
    template <typename T, typename U>
    FieldContentVariant operator()( const T &, const U& ) const {return false;}
};

struct variant_str : public boost::static_visitor<std::string> {
    std::string operator()() const { return ""; }
    template <typename T>
    std::string operator()(const T t) const { 
        return str_cat<T>(t);
    }
};

struct variant_typeid : public boost::static_visitor<const std::type_info&> {
    const std::type_info& operator()() const { return typeid(void); }
    template <typename T>
    const std::type_info& operator()(const T t) const {
        return typeid(T);
    }
};

struct variant_hash : public boost::static_visitor<size_t> {
    size_t operator()() const { return 0; }
    template <typename T>
    size_t operator()(const T t) const { 
        return std::hash<T>()(t);
    }
};

struct visit_setter : public boost::static_visitor<void> {
    visit_setter(Message* m, const FieldDescriptor* f) : m(m), f(f), r(m->GetReflection()) {};
    Message* m; const FieldDescriptor* f; const Reflection* r;
    void operator()(int32_t i) const { r->SetInt32(m, f, i); };
    void operator()(int64_t i) const { r->SetInt64(m, f, i); };
    void operator()(uint32_t i) const { r->SetUInt32(m, f, i); };
    void operator()(uint64_t i) const { r->SetUInt64(m, f, i); };
    void operator()(double i) const { r->SetDouble(m, f, i); };
    void operator()(float i) const { r->SetFloat(m, f, i); };
    void operator()(bool i) const { r->SetBool(m, f, i); };
    void operator()(std::string i) const { r->SetString(m, f, i); };
};


struct visit_adder : public boost::static_visitor<void> { 
    visit_adder(Message* m, const FieldDescriptor* f) : m(m), f(f), r(m->GetReflection()) {};
    Message* m; const FieldDescriptor* f; const Reflection* r;
    void operator()(int32_t i) const { r->AddInt32(m, f, i); };
    void operator()(int64_t i) const { r->AddInt64(m, f, i); };
    void operator()(uint32_t i) const {
        if (f->cpp_type() == FieldDescriptor::CPPTYPE_ENUM) {
            const EnumValueDescriptor* evd = f->default_value_enum()->type()->FindValueByNumber(i);
            r->AddEnum(m, f, evd);
        } else {
            r->AddUInt32(m, f, i); 
        }
    };
    void operator()(uint64_t i) const { r->AddUInt64(m, f, i); };
    void operator()(double i) const { r->AddDouble(m, f, i); };
    void operator()(float i) const { r->AddFloat(m, f, i); };
    void operator()(bool i) const { r->AddBool(m, f, i); };
    void operator()(std::string i) const { r->AddString(m, f, i); };
};

// TODO: Find better way to hash/compare Messages than the DebugString
class FieldContent {
    public:
        FieldContent(const Message& m, const FieldDescriptor* f, int i) {
            _message = false;
            const Reflection* r = m.GetReflection();
            switch (f->cpp_type()) {
                case FieldDescriptor::CPPTYPE_INT32: content = r->GetRepeatedInt32(m, f, i); break;
                case FieldDescriptor::CPPTYPE_INT64: content = r->GetRepeatedInt64(m, f, i); break;
                case FieldDescriptor::CPPTYPE_UINT32: content = r->GetRepeatedUInt32(m, f, i); break;
                case FieldDescriptor::CPPTYPE_UINT64: content = r->GetRepeatedUInt64(m, f, i); break;
                case FieldDescriptor::CPPTYPE_DOUBLE: content = r->GetRepeatedDouble(m, f, i); break;
                case FieldDescriptor::CPPTYPE_FLOAT: content = r->GetRepeatedFloat(m, f, i); break;
                case FieldDescriptor::CPPTYPE_BOOL: content = r->GetRepeatedBool(m, f, i); break;
                case FieldDescriptor::CPPTYPE_ENUM: content = (uint32_t)r->GetRepeatedEnum(m, f, i)->number(); break;
                case FieldDescriptor::CPPTYPE_STRING: content = r->GetRepeatedString(m, f, i); break;
                case FieldDescriptor::CPPTYPE_MESSAGE: 
                    content_msg = &r->GetRepeatedMessage(m, f, i);
                    content_debug_str = content_msg->DebugString();
                    _message = true;
                    break;
                default:
                    FATAL("Unknown type ", f->cpp_type());
            };
        }
        FieldContent(const Message& m, const FieldDescriptor* f) {
            const Reflection* r = m.GetReflection();
            _message = false;
            switch (f->cpp_type()) {
                case FieldDescriptor::CPPTYPE_INT32: content = r->GetInt32(m, f); break;
                case FieldDescriptor::CPPTYPE_INT64: content = r->GetInt64(m, f); break;
                case FieldDescriptor::CPPTYPE_UINT32: content = r->GetUInt32(m, f); break;
                case FieldDescriptor::CPPTYPE_UINT64: content = r->GetUInt64(m, f); break;
                case FieldDescriptor::CPPTYPE_DOUBLE: content = r->GetDouble(m, f); break;
                case FieldDescriptor::CPPTYPE_FLOAT: content = r->GetFloat(m, f); break;
                case FieldDescriptor::CPPTYPE_BOOL: content = r->GetBool(m, f); break;
                case FieldDescriptor::CPPTYPE_ENUM: content = (uint32_t)r->GetEnum(m, f)->number(); break;
                case FieldDescriptor::CPPTYPE_STRING: content = r->GetString(m, f); break;
                case FieldDescriptor::CPPTYPE_MESSAGE:
                    content_msg = &r->GetMessage(m, f);
                    content_debug_str = content_msg->DebugString();
                    _message = true;
                    break;
                default:
                    FATAL("Unknown type ", f->cpp_type());
            };
        }
        
        bool operator<(const FieldContent& rhs) const {
            return content < rhs.content;
        }

        FieldContent(const FieldContentVariant& v) {
            content = v;
            _message = false;
        }

        FieldContent operator+(const FieldContent& rhs) const {
            assert (!_message);
            return boost::apply_visitor(variant_adder(), content, rhs.content);
        };

        FieldContent operator*(const FieldContent& rhs) const {
            assert (!_message);
            return boost::apply_visitor(variant_multiplier(), content, rhs.content);
        }

        // From CERN ROOT Rtypes.h
        typedef unsigned long long ULong64_t;

        template<typename T>
        operator T() const {
            assert (!_message);
            // Workaround for compiler+architecture combinations which don't
            // consider uint64_t and ULong64_t compatible
            if (typeid(T) == typeid(ULong64_t))
                return boost::get<uint64_t>(content);
            return boost::get<T>(content);
        }
        
        bool operator==(const FieldContent& rhs) const {
            if (_message) {
                return rhs.content_debug_str == content_debug_str;
            } else {
                return rhs.content == content;
            }
        }

        size_t hash_value() const {
            if (_message) {     
                return std::hash<std::string>()(content_debug_str);
            } else {
                return boost::apply_visitor(variant_hash(), content);
            }
        }
        
        const std::type_info& type() const {
            if (_message) {
                return typeid(void);
            }
            return boost::apply_visitor(variant_typeid(), content);
        }
        
        std::string str() const {
            if (_message) {
                return content_debug_str;
            } else {
                return boost::apply_visitor(variant_str(), content);
            }
        }

    private:
        bool _message;
        FieldContentVariant content;
        const Message* content_msg;
        std::string content_debug_str;
        friend class DynamicField;

};

class ConstDynamicField {
    public:
    
        ConstDynamicField(const Message& m, const FieldDescriptor* f) : _m(m), _f(f), _r(m.GetReflection()) {}

        bool repeated() const { return _f->is_repeated(); }

        bool message() const { return _f->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE; }
        
        FieldDescriptor::CppType cpp_type() const { return _f->cpp_type(); }

        const std::string& name() const { return _f->full_name(); }
        
        bool present() const { return repeated() ? size() != 0 : _r->HasField(_m, _f); }

        FieldContent value() const {
            assert(!repeated());
            return FieldContent(_m, _f);
        }

        FieldContent value(int i) const {
            assert(repeated());
            return FieldContent(_m, _f, i);
        }
        
        
        const Message& submessage() const {
            assert(not repeated());
            return _r->GetMessage(_m, _f);
        }
        
        const Message& submessage(int i) const {
            assert(repeated());
            return _r->GetRepeatedMessage(_m, _f, i);
        }
        
        MetadataMergeOptions merge_option() const {
            return _f->options().GetExtension(merge);
        }

        int size() const {
            assert(repeated());
            return _r->FieldSize(_m, _f);
        }

        bool operator==(const ConstDynamicField& rhs) const {
            if (repeated()) {
                if(size() != rhs.size()) 
                    return false;
                for (int i = 0; i < size(); i++) {
                    if (!(value(i) == rhs.value(i))) 
                        return false;
                }
                return true;
            } else {
                return value() == rhs.value();
            }
        }

        void assert_compatible(const ConstDynamicField& rhs) {
            if (_m.GetDescriptor() != rhs._m.GetDescriptor())
                FATAL("Unequal message descriptors!");
            if (_f != rhs._f)
                FATAL("Unequal field descriptors!");
            if (_f->containing_type() != _m.GetDescriptor())
                FATAL("Field is not contained by this message!");
        }

    protected:
        const Message& _m;
        const FieldDescriptor* _f;
        const Reflection* _r;
};

class DynamicField : public ConstDynamicField {
    public:
        DynamicField(Message& m, const FieldDescriptor* f) 
            : ConstDynamicField(m, f), _m_mutable(m) {}
        
        void set(const FieldContent& c) {
            assert(!repeated());
            boost::apply_visitor(visit_setter(&_m_mutable, _f), c.content);
        }
        
        void clear() {
            _r->ClearField(&_m_mutable, _f);
        }

        void add(const FieldContent& c) {
            assert(repeated());
            if (c._message) {
                _r->AddMessage(&_m_mutable, _f)->CopyFrom(*(c.content_msg));
            } else {
                boost::apply_visitor(visit_adder(&_m_mutable, _f), c.content);
            }
        }

    private:
        Message& _m_mutable;
};

void add_fields(const ConstDynamicField& f1, const ConstDynamicField& f2, DynamicField& merged);
void multiply_fields(const ConstDynamicField& f1, const ConstDynamicField& f2, DynamicField& merged);
void append_fields(const ConstDynamicField& f1, const ConstDynamicField& f2, DynamicField& merged, bool make_unique);

void inplace_add_fields(DynamicField& merged, ConstDynamicField & f2);
void inplace_multiply_fields(DynamicField& merged, ConstDynamicField& f2);
void inplace_append_fields(DynamicField& merged, ConstDynamicField& f2);

}
}

namespace std {
    template <>
    struct hash<a4::io::FieldContent> {
        std::size_t operator()(const a4::io::FieldContent& fc) const {
            return fc.hash_value();
        }
    };
}

#endif
