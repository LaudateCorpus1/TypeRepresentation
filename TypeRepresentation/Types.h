#pragma once

#include <string>
#include <vector>
#include <unordered_map>

namespace Types
{
    enum Primitive
    {
        Int8,
        Uint8,
        Int16,
        Uint16,
        Int32,
        Uint32,
        Int64,
        Uint64,
        Dsint,
        Duint,
        Pointer,
        Float,
        Double
    };

    struct Type
    {
        std::string name; //Type identifier.
        Primitive primitive; //Primitive type.
        int bitsize = 0; //Size in bits.
        std::string pointto; //Type identifier of *Type
    };

    struct Member
    {
        std::string name; //Member identifier
        std::string type; //Type.name
        int offset = 0; //Member offset in Struct (ignored for unions)
        int arrsize = 0; //Number of elements if Member is an array
    };

    struct StructUnion
    {
        bool isunion = false; //Is this a union?
        int alignment = sizeof(void*); //StructUnion alignment
        std::string name; //StructUnion identifier
        std::vector<Member> members; //StructUnion members
    };

    struct TypeManager
    {
        explicit TypeManager()
        {
            setupPrimitives();
        }

        bool AddType(const std::string & name, const std::string & primitive)
        {
            auto found = primitives.find(primitive);
            if (found == primitives.end())
                return false;
            return AddType(name, found->second);
        }

        bool AddType(const std::string & name, Primitive primitive, int bitsize = 0, const std::string & pointto = "")
        {
            if (isDefined(name))
                return false;
            Type t;
            t.name = name;
            t.primitive = primitive;
            auto primsize = primitivesizes[primitive];
            if (bitsize <= 0)
                t.bitsize = primsize;
            else if (bitsize > primsize)
                return false;
            else
                t.bitsize = bitsize;
            t.pointto = pointto;
            return AddType(t);
        }

        bool AddType(const Type & t)
        {
            if (isDefined(t.name))
                return false;
            types.insert({ t.name, t });
            return true;
        }

        bool AddStruct(const std::string & name, int alignment = 0)
        {
            StructUnion s;
            s.name = name;
            if (alignment > 0)
                s.alignment = alignment;
            return AddStruct(s);
        }

        bool AddStruct(const StructUnion & s)
        {
            laststruct = s.name;
            if (isDefined(s.name))
                return false;
            structs.insert({ s.name, s });
            return true;
        }

        bool AddUnion(const std::string & name, int alignment = 0)
        {
            StructUnion u;
            u.isunion = true;
            u.name = name;
            if (alignment > 0)
                u.alignment = alignment;
            return AddUnion(u);
        }

        bool AddUnion(const StructUnion & u)
        {
            laststruct = u.name;
            if (isDefined(u.name))
                return false;
            structs.insert({ u.name, u });
            return true;
        }

        bool AppendMember(const std::string & name, const std::string & type, int offset = -1, int arrsize = 0)
        {
            return AddMember(laststruct, name, type, offset, arrsize);
        }

        bool AddMember(const std::string & parent, const std::string & name, const std::string & type, int offset = -1, int arrsize = 0)
        {
            auto found = structs.find(parent);
            if (arrsize < 0 || found == structs.end() || !isDefined(type))
                return false;
            auto & s = found->second;

            for (const auto & member : s.members)
                if (member.name == name)
                    return false;

            Member m;
            m.name = name;
            m.arrsize = arrsize;
            m.type = type;
            if (offset < 0)
            {
                int alignment;
                m.offset = getSizeof(parent, &alignment, 0);
                if (s.members.size() && !m.offset)
                    return false;
                auto typesize = Sizeof(type);
                if (alignment && typesize <= alignment)
                    m.offset -= alignment;
            }
            else
                m.offset = offset;

            s.members.push_back(m);
            return true;
        }

        bool AddMember(const std::string & parent, const Member & member)
        {
            auto found = structs.find(parent);
            if (found == structs.end())
                return false;
            found->second.members.push_back(member);
            return true;
        }

        int Sizeof(const std::string & type)
        {
            return getSizeof(type, nullptr, 0);
        }

    private:
        std::unordered_map<std::string, Primitive> primitives;
        std::unordered_map<Primitive, int> primitivesizes;
        std::unordered_map<std::string, Type> types;
        std::unordered_map<std::string, StructUnion> structs;
        std::string laststruct;

        void setupPrimitives()
        {
            auto p = [this](const std::string & n, Primitive p, int bitsize)
            {
                std::string a;
                for (auto ch : n)
                {
                    if (ch == ',')
                    {
                        if (a.length())
                            primitives.insert({ a, p });
                        a.clear();
                    }
                    else
                        a.push_back(ch);
                }
                if (a.length())
                    primitives.insert({ a, p });
                primitivesizes[p] = bitsize;
            };
            p("int8_t,int8,char,byte,bool,signed char", Int8, sizeof(char) * 8);
            p("uint8_t,uint8,uchar,unsigned char,ubyte", Uint8, sizeof(unsigned char) * 8);
            p("int16_t,int16,wchar_t,char16_t,short", Int16, sizeof(short) * 8);
            p("uint16_t,uint16,ushort,unsigned short", Int16, sizeof(unsigned short) * 8);
            p("int32_t,int32,int,long", Int32, sizeof(int) * 8);
            p("uint32_t,uint32,unsigned int,unsigned long", Uint32, sizeof(unsigned int) * 8);
            p("int64_t,int64,long long", Int64, sizeof(long long) * 8);
            p("uint64_t,uint64,unsigned long long", Uint64, sizeof(unsigned long long) * 8);
            p("dsint", Dsint, sizeof(void*) * 8);
            p("duint,size_t", Duint, sizeof(void*) * 8);
            p("ptr,void*", Pointer, sizeof(void*) * 8);
            p("float", Float, sizeof(float) * 8);
            p("double", Double, sizeof(double) * 8);
        }

        template<typename K, typename V>
        static bool mapContains(const std::unordered_map<K, V> & map, const K & k)
        {
            return map.find(k) != map.end();
        }

        bool isDefined(const std::string & id) const
        {
            return mapContains(primitives, id) || mapContains(types, id) || mapContains(structs, id);
        }

        int getSizeof(const std::string & type, int* alignment, int depth)
        {
            if (depth >= 100)
                return 0;
            if (alignment)
                *alignment = 0;
            auto foundP = primitives.find(type);
            if (foundP != primitives.end())
                return primitivesizes[foundP->second] / 8;
            auto foundT = types.find(type);
            if (foundT != types.end())
            {
                auto bitsize = foundT->second.bitsize;
                auto mod = bitsize % 8;
                if (mod)
                    bitsize += 8 - mod;
                return bitsize / 8;
            }
            auto foundS = structs.find(type);
            if (foundS == structs.end() || !foundS->second.members.size())
                return 0;
            const auto & s = foundS->second;
            auto size = 0;
            if (foundS->second.isunion)
            {
                for (const auto & member : s.members)
                {
                    auto membersize = getSizeof(member.type, nullptr, depth + 1) * (member.arrsize ? member.arrsize : 1);
                    if (!membersize)
                        return 0;
                    if (membersize > size)
                        size = membersize;
                }
            }
            else
            {
                const auto & last = s.members[s.members.size() - 1];
                auto lastsize = getSizeof(last.type, nullptr, depth + 1) * (last.arrsize ? last.arrsize : 1);
                if (!lastsize)
                    return 0;
                size = last.offset + lastsize;
            }
            auto mod = size % s.alignment;
            if (mod)
            {
                if (alignment)
                    *alignment = s.alignment - mod;
                size += s.alignment - mod;
            }
            return size;
        }
    };
};
