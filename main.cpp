#include <cstdio>
#include <cassert>
#include <string>
#include <sstream>
#include <vector>

struct Name_Const_Char
{
    constexpr Name_Const_Char(const char* str): data(str) {}

    constexpr bool operator == (const Name_Const_Char str) const
    {
        auto c {str.data}; 
        for(; *c != 0; c++)
        {
            if(*c != data[c - str.data]){
                return false;
            }
        }
        return !data[c - str.data];
    }

    const char* data;
};

template <size_t N>
struct Name
{
    constexpr Name(const char (&str)[N])
    { 
        for(int i = 0; i < N; i++){
            data[i] = str[i];
        }
    }
    constexpr bool operator == (const Name& n) const
    {
        if constexpr (n.len != len){
            return false;
        }

        auto c {n.data}; 
        for(; *c != 0; c++)
        {
            if(*c != data[c - n.data]){
                return false;
            }
        }
        return !data[c - n.data];
    }

    constexpr bool operator == (const char* str) const
    {
        auto c {str}; 
        for(; *c != 0; c++)
        {
            if(*c != data[c - str]){
                return false;
            }
        }
        return !data[c - str];
    }

    constexpr const char* cstr() const
    {
        return data;
    }

    static constexpr size_t len {N};
    char data[N];
};

template <class C, typename U>
U pointer_to_member_type (U C::*v);

template<typename T>
struct Pointer_To_Member
{
    constexpr Pointer_To_Member(const T t){
        type = t;
    }

    template<typename U>
    constexpr bool operator == (const Pointer_To_Member<U>& m)
    {
        if constexpr(std::is_same_v<T, U>){
            return type == m.type;
        }
        return false;
    }

    T type;
};

template<Pointer_To_Member U, Name N>
struct Reflectable
{
    constexpr Reflectable () {}
    template<typename T>
    constexpr auto& operator()(T& t) const
    {
        return t.*type;
    }
    static constexpr auto pointer()
    {
        return U;
    }
    using value_type = decltype(pointer_to_member_type(U.type));
    static constexpr const decltype(U.type) type {U.type};
    static constexpr const char* name {N.cstr()};
    static constexpr const Name_Const_Char cc_name {N.cstr()};
};

#define PARENS ()

#define EXPAND(...) EXPAND4(EXPAND4(EXPAND4(EXPAND4(__VA_ARGS__))))
#define EXPAND4(...) EXPAND3(EXPAND3(EXPAND3(EXPAND3(__VA_ARGS__))))
#define EXPAND3(...) EXPAND2(EXPAND2(EXPAND2(EXPAND2(__VA_ARGS__))))
#define EXPAND2(...) EXPAND1(EXPAND1(EXPAND1(EXPAND1(__VA_ARGS__))))
#define EXPAND1(...) __VA_ARGS__

#define FOR_EACH_MEMBER(...) __VA_OPT__(EXPAND(FOR_EACH_MEMBER_NEXT(__VA_ARGS__)))

#define FOR_EACH_MEMBER_NEXT(a, ...) Reflectable<&meta_tag::a, #a> __VA_OPT__(,) __VA_OPT__(FOR_EACH_MEMBER_NEXT2 PARENS (__VA_ARGS__))

#define FOR_EACH_MEMBER_NEXT2() FOR_EACH_MEMBER_NEXT 

#define Reflect(name, ...) using meta_tag = name; static constexpr Reflect_Data<meta_tag, #name, FOR_EACH_MEMBER(__VA_ARGS__)> meta{}

// this is good enough for now...
template<typename ...Ts>
constexpr bool no_duplicate_members()
{
    constexpr const char* names[sizeof...(Ts)] {Ts{}.name...};

    for(int i = 0; i < sizeof...(Ts); i++)
    {
        for(int j = 0; j < sizeof...(Ts); j++)
        {
            if(i != j)
            {
                Name_Const_Char a {names[i]};
                Name_Const_Char b {names[j]};
                if(a == b){
                    return false;
                }
            }
        }
    }

    return true;
}

template<typename ...Ts> 
struct Members 
{
    constexpr Members() {}

    static constexpr const char* names[sizeof...(Ts)] {Ts{}.name...};

    static_assert(no_duplicate_members<Ts...>(), "error when creating members meta data: list has duplicate members");

    static constexpr size_t count {sizeof...(Ts)};

    template<Name N, typename T, typename ...Us>
    constexpr auto by_name_helper() const
    {
        if constexpr (N == T{}.name){
            return T{};
        }
        else
        {
            return [this]<typename First, typename... Rest>(First f, Rest... r)
            {
                return by_name_helper<N, Rest...>();
            }(T{}, Us{}...);
        }
    }

    template<Name N>
    constexpr auto by_name() const
    {
        return by_name_helper<N, Ts...>();
    }

    template<size_t I, size_t Current = 0>
    constexpr auto at() const
    {
        static_assert(Current <= I, "at error : index out of bounds");
        return []<typename T, typename ...Us>(T t, Us... us)
        {
            if constexpr(Current != I){
                return Members<Us...>{}.template at<I, Current + 1>();
            }
            else{
                return T{};
            }

        }(Ts{}...);
    }

    template<size_t I>
    constexpr auto get() const
    {
        return at<I>(); 
    }
};

template<size_t index = 0>
constexpr auto for_each(auto& s, auto t)
{
    constexpr auto v {s.meta.members.template at<index>()};
    t(s, v);

    if constexpr(index + 1 < s.meta.members.count){
        for_each<index + 1>(s, t);
    }
}

template<typename T, Name N, typename ...Ts>
struct Reflect_Data
{
    constexpr Reflect_Data() {}
    using type = T;
    static constexpr const char* name {N.cstr()};
    static constexpr Members<Ts...> members;
};

struct RGBA
{
    float r {1};
    float g {3};
    float b {5};
    float a {7};

    std::string name {"skyblue"};

    Reflect(RGBA, r, g, b, a, name);
};

struct Vec
{
    float x {0};
    float y {0};

    Reflect(Vec, x, y);
};

struct Foo
{
    int bar;
    int zed;
    float test;
    float test2;
    float test3;
    Vec position;
    Vec heading;
    Vec origin;
    RGBA background;
    RGBA highlight;

    Reflect(Foo, bar,
                 zed,
                 position,
                 heading,
                 origin,
                 background,
                 highlight);
};

// naively converts an object to a string
// can be good for serialization
template<size_t I = 0, bool Nested = false>
std::string to_string(const auto& s)
{
    std::string result;
    if constexpr(requires{s.meta.members;})
    {
        constexpr auto member {s.meta.members.template at<I>()};
        if constexpr(I == 0){
            result += '{';
        }
        auto& v {member(s)};
        result += to_string<0, requires{v.meta.members;}>(v);
        if constexpr(I + 1 < s.meta.members.count)
        {
            result += ", ";
            result += to_string<I + 1>(s);
        }
        else{
            result += '}';
        }
    }
    // assume s here is now a POD after all of the recursion, if can't be converted to a string, this won't work
    else
    { 
        if constexpr(Nested)
        {
            // hack to check if s is an std::string
            if constexpr(requires{s == std::string{};}){
                result += '"' + s + '"' + ", ";
            }
            else
            {
                if constexpr(requires{s.begin();} && requires{s.end();})
                {
                    result += "{";
                    for(auto& i : s){
                        result += to_string< 0, requires {i.meta.members;}>(i) + ", ";
                    }
                    result.pop_back();
                    result.pop_back();
                    result += "}, ";
                }
                else{
                    result += std::to_string(s) + ", ";
                }
            }
        } 
        else
        {
            if constexpr(requires{s == std::string{};}){
                result += '"' + s + '"';
            }
            else
            {
                if constexpr(requires{s.begin();} && requires{s.end();})
                {
                    result += "{";
                    for(auto& i : s){
                        result += to_string< 0, requires {i.meta.members;}>(i) + ", ";
                    }
                    result.pop_back();
                    result.pop_back();
                    result += "}";
                }
                else{
                    result += std::to_string(s);
                }
            }
        } 
    }
    return result;
}

// reverse engineer to_string function
template<size_t I = 0, bool Nested = false>
void from_string_helper(auto& s, std::string& str)
{
    if(str.empty()){
        return;
    }
    if constexpr(requires{s.meta.members;})
    {
        if(str.front() == '{'){
            str.erase(0, 1);
        }

        constexpr auto member {s.meta.members.template at<I>()};
        auto& v {member(s)};
        from_string_helper<0, requires{v.meta.members;}>(v, str); 

        if constexpr(I + 1 < s.meta.members.count){
            from_string_helper<I + 1>(s, str);
        }
        else{
            str.erase(0, 1);
        }
    }
    else
    {
        // assume s here is now a POD after all of the recursion, if can't be converted from a string, this won't work
        
        auto comma {str.find_first_of(",")};
        if(comma != str.npos){
            str.erase(comma, 1);
        }

        // hack to check if s is an std::string
        if constexpr(requires{s == std::string{};})
        {
            auto quote {str.find('"')};
            if(quote != str.npos)
            {
                auto end_quote {str.find('"', quote + 1)};
                if(end_quote != str.npos)
                {
                    s = str.substr(quote + 1, end_quote - 1);
                    str.erase(quote, end_quote + 1);
                }
                else{
                    // assert here or something
                }
            }
        }
        else
        {
            if constexpr(requires{s.begin();} && requires{s.end();})
            {
                if(str.front() == '{'){
                    str.erase(0, 1);
                }
                s = {};
                using T = std::remove_reference<decltype(s)>::type::value_type;
                std::stringstream ss {str};
                std::string token;
                if constexpr(requires{T::meta.members;} || (requires{T{}.begin();} && requires{T{}.end();}))
                {
                    while(std::getline(ss, token, '}'))
                    {
                        s.push_back({});
                        from_string_helper<0, true>(s.back(), token);
                        str.erase(0, token.length() + 3);
                        ss.ignore();
                        ss.ignore();
                        ss.ignore();
                    }
                }
                else
                {
                    while(ss>>token)
                    {
                        s.push_back({});
                        from_string_helper<0, requires{T::meta.members;}>(s.back(), str);
                    }
                }
            }
            else
            {
                std::stringstream ss {str};
                ss>>s;
            }
        }

        auto space {str.find_first_of(" ")};
        if(space != str.npos){
            str.erase(0, space + 1);
        }
        if(str.front() == '}'){
            str.erase(0, 1);
        }
    }
}

void from_string(auto* s, const std::string& str)
{
    auto str_copy {str};
    from_string_helper(*s, str_copy);
}

// prints a whole struct with name and names of members and their values
// if you don't want to convert to string, you can use std::print of course
template<size_t I = 0>
void pretty_print(auto& s) requires requires {s.meta.members;}
{
    if constexpr(I == 0){
        printf("struct %s {", s.meta.name);
    }

    constexpr auto member {s.meta.members.template at<I>()};
    auto& v {member(s)};

    // assuming v can be converted to a string
    printf("%s : %s", member.name, to_string(v).c_str());

    if constexpr(I + 1 < s.meta.members.count)
    {
        printf(",\n");
        pretty_print<I + 1>(s);
    }

    if constexpr(I == 0){
        printf("}");
    }
}

void pretty_printnl(auto& s) requires requires {s.meta.members;}
{
    pretty_print(s);
    printf("\n");
}

void ugly_printnl(auto& s)
{
    printf("%s", to_string(s).c_str());
    printf("\n");
}

struct Empty {};

template<typename Parent, typename T, size_t S, typename From>
struct SOA_Base : Parent
{
    static constexpr const char* name {T{}.name};

    T::value_type data[S]{};

    template<Name N>
    constexpr auto field() const{
        return field_impl<N>();
    }

    template<Name N>
    constexpr auto field(){
        return field_impl<N>();
    }

    template<Pointer_To_Member P>
    constexpr auto field()
    {
        if constexpr(P == T::pointer()){
            return get_data();
        }
        else
        {
            auto i {static_cast<Parent*>(this)};
            return i->template field<P>();
        }
    }

    template<Pointer_To_Member P>
    constexpr auto field() const
    {
        if constexpr(P == T::pointer()){
            return get_data();
        }
        else
        {
            auto i {static_cast<Parent*>(this)};
            return i->template field<P>();
        }
    }

    template<Name N>
    constexpr auto field_impl()
    {
        if constexpr(name == N){
            return get_data();
        }
        else
        {
            auto i {static_cast<Parent*>(this)};
            return i->template field_impl<N>();
        }
    }

    constexpr auto get_data()
    {
        struct View
        {
            constexpr View(T::value_type* data){
                view = data;
            }
            constexpr auto& operator[](size_t i)
            {
                assert(i < S);
                return view[i];
            }
            constexpr auto begin(){
                return view;
            }
            constexpr auto end(){
                return view + S;
            }
            constexpr auto raw(){
                return view;
            }
            T::value_type* view {nullptr};
        };
        return View{data};
    }
};

template<size_t S, typename From, Name N, typename T = Empty, typename ...Ts>
constexpr auto new_soa(const Reflect_Data<From, N, Ts...> members)
{
    if constexpr(sizeof...(Ts) == 0){
        return T{};
    }
    else
    {
        return [&]<typename First, typename ...Rest>(First f, Rest ...rs)
        {
            return new_soa<S, From, N, SOA_Base<T, decltype(Members<Ts...>{}.template at<0>()), S, From>, Rest...>(Reflect_Data<From, N, Rest...>{});
        }(Ts{}...);
    }
}

template<size_t S>
constexpr auto create_soa(const auto& from)
{
    return new_soa<S>(from.meta);
}

template<size_t S>
constexpr auto make_soa(const auto& from)
{
    return [&]<typename ...Ts>(const Members<Ts...>&){
        new_soa<S>(Reflect_Data<decltype(from), from.baked_meta_name, Ts...>{});
    }(from.meta_members);
    return new_soa<S>(from.meta);
}

struct Test
{
    int x;
    int y;
    int gold;
    int health;

    Reflect(Test, x, y);
};

void general_test()
{
    Foo foo;

    foo.bar = 5;
    foo.zed = 1;
    foo.position = {1, 2};
    foo.heading = {6, 9};
    foo.origin = {4, 20};

    // get feild data at compile time by name or index
    constexpr auto position {Foo::meta.members.by_name<"position">()};
    position(foo).x = 123;
    position(foo).y = 456;

    printf("position is {%f, %f}\n", foo.position.x, foo.position.y);
    printf("old origin is {%f, %f}\n", foo.origin.x, foo.origin.y);
    printf("old zed is %i\n", foo.zed);

    std::string new_origin {"{69, 420}"};

    for_each(foo, [&](auto& object, auto member)
    {
        if constexpr(requires{member(object) + 1;}){
            printf("member %s has + operator\n", member.name);
        }

        // select members by name at run time
        if(std::string{"origin"} == member.name)
        {
            from_string(&member(object), new_origin);
            return;
        }
        if(std::string{"zed"} == member.name)
        {
            from_string(&member(object), std::to_string((int)'z'));
            return;
        }
    });

    printf("new origin is {%f, %f}\n", foo.origin.x, foo.origin.y);
    printf("new zed is %i\n", foo.zed);

    printf("\npretty print :\n");
    pretty_printnl(foo);
    printf("\n");
    
    printf("ugly print :\n");
    ugly_printnl(foo);

    printf("\ncolor from string pretty print :\n");

    std::string some_color {R"({123, 25, 73, 255, "some color"})"};
    from_string(&foo.background, some_color);
    pretty_printnl(foo.background);

    printf("\nfoo from string pretty print :\n");

    auto foo_string {to_string(foo)};
    Foo foo_copy;
    from_string(&foo_copy, foo_string);
    pretty_printnl(foo_copy);

    {
        {
            std::vector<int> ints {1, 2, 3, 4, 5};

            ugly_printnl(ints);

            std::string ints_string {R"({420, 69, 322})"};
            from_string(&ints, ints_string);

            ugly_printnl(ints);
        }

        {
            std::string vecs_string {"{{1, 2}, {3, 4}, {5, 6}}"};
            std::vector<Vec> vecs {};
            from_string(&vecs, vecs_string);
            ugly_printnl(vecs);
        }

        {
            std::string vecs_int_string {"{}"};
            std::vector<int> vecs_int {};
            from_string(&vecs_int, vecs_int_string);
            ugly_printnl(vecs_int);
        }

        {
            std::string vecs_int_string {"{1, 2, 3}"};
            std::vector<int> vecs_int {};
            from_string(&vecs_int, vecs_int_string);
            ugly_printnl(vecs_int);
        }

        {
            std::string vecs_int_string {"{{1, 2, 3}, {4, 5, 6}}"};
            std::vector<std::vector<int>> vecs_int {};
            from_string(&vecs_int, vecs_int_string);
            ugly_printnl(vecs_int);
        }

        {
            std::string vecs_int_string {"{{{1, 2}, {3, 4}}, {{5, 6, 7}, {9, 10, 11}}, {{100, 200, 300}, {400, 500, 600}}}"};
            std::vector<std::vector<std::vector<int>>> vecs_int {};
            from_string(&vecs_int, vecs_int_string);
            ugly_printnl(vecs_int);
        }
    }
}

template<auto T>
struct Constant
{
    static constexpr decltype(T) value {T};
    constexpr operator auto ()
    {
        return T;
    }
};

template<auto T>
struct Exit
{
    static constexpr decltype(T) value {T};

    template<auto U>
    constexpr bool operator == (const Exit<U> e)
    {
        return T == e;
    }
};

template<size_t I>
struct CT_Loop
{
    constexpr CT_Loop() {}

    template<typename L, typename Inc>
    struct Ret
    {
        constexpr Ret(L l, Inc ic) : c(l), inc(ic){}

        auto operator = (auto t)
        {
            if constexpr(requires {t.template operator()<I>() == Exit<true>{};}){
                return t.template operator()<I>();
            }
            else{
                t.template operator()<I>();
            }
            constexpr auto iv {decltype(inc.template operator()<I>())::value};
            if constexpr(decltype(c.template operator()<iv>())::value){
                CT_Loop<iv>{}(c, inc) = t;
            }
        }
        L c;
        Inc inc;
    };

    template<typename C, typename Inc>
    constexpr auto operator()(C c, Inc inc) 
    {
        return Ret<C, Inc>{c, inc};
    }
};

#define breakc return Exit<true>{}
#define forc(init, cond, inc) CT_Loop<[&]{\
                                        constexpr auto f = [&]<init>{constexpr auto o {inc}; return o;};\
                                        constexpr auto v {f()}; \
                                        constexpr auto v2 {f.template operator()<v>()}; \
                                        return v - (v2 - v); \
                                        }()>{}([&]<init>{return Constant<(cond)>{};}, [&]<init>{return Constant<inc>{};}) = [&]<init>


static constexpr const char* FORMAT_SPECIFIERS[] {"%c", "%i", "%u", "%f", "%s", "%p"};

template<typename T>
consteval auto get_format_specifier(const T& t)
{
    if constexpr(std::is_same_v<T, const char*> || requires{ t == (const char*){nullptr} ;}){
        return FORMAT_SPECIFIERS[4];
    }
    else if constexpr(std::is_same_v<T, char>){
        return FORMAT_SPECIFIERS[0];
    }
    else if constexpr(std::is_floating_point_v<T>){
        return FORMAT_SPECIFIERS[3];
    }
    else if constexpr(std::is_signed_v<T>){
        return FORMAT_SPECIFIERS[1];
    }
    else if constexpr(std::is_unsigned_v<T>){
        return FORMAT_SPECIFIERS[2];
    }
    else if constexpr(std::is_pointer_v<T>){
        return FORMAT_SPECIFIERS[5];
    }
    else
    {
        struct Invalid_Specifier{};

        return Invalid_Specifier{};
    }
}

template<typename ...T>
struct Formatter
{
    template<size_t N>
    consteval Formatter(const char (&str)[N])
    {
        int ec {0};
        for(int i = 0; i < N; i++)
        {
            if(i == 0)
            {
                ec++;
                continue;
            }
            if(str[i] == '%' && (i + 1 >= N || str[i + 1] != '%'))
            {
                if(str[i - 1] != '%'){
                    ec++;
                }
            }
        }
        if(ec < sizeof...(T)){
            throw "format string element count does not match argument count";
        }
        ptr = str;
    }
    template<bool nl = false>
    void format()
    {
        fmt = ptr;
        constexpr const char* fmts[] {get_format_specifier(T{})...};
        int ec {0};
        for(int i = 0; i < fmt.length(); i++)
        {
            if(i == 0 || (fmt[i] == '%' && (i + 1 >= fmt.length() || fmt[i + 1] != '%')))
            {
                if(i == 0 || fmt[i - 1] != '%')
                {
                    fmt.replace(i, 1, fmts[ec]);
                    i += std::string_view{fmts[ec]}.length() - 1;
                    ec++;
                }
                else
                {
                    fmt.replace(i, 1, "%%");
                    i += 2;
                }
            }
        }
        if constexpr(nl){
            fmt += '\n';
        }
    }
    const char* ptr;
    std::string fmt;
};

template<typename ...T>
using Format_String = std::type_identity_t<Formatter<T...>>;

template<typename ...T>
void print(Format_String<T...> fmt, const T&... ts)
{
    fmt.format();
    printf(fmt.fmt.c_str(), ts...);
}

template<typename ...T>
void println(Format_String<T...> fmt, const T&... ts)
{
    fmt.template format<true>();
    printf(fmt.fmt.c_str(), ts...);
}

template<typename T, Name N, typename ...Ts>
struct Meta_Data
{
    constexpr Meta_Data() {}
    using type = T;
    static constexpr const char* name {N.cstr()};
    static constexpr Members<Ts...> members;
};

template<typename T, typename U>
struct Pair
{
    using first_type = T;
    using second_type = U;
};

template<Name n, typename ...T>
struct Meta_Struct_Impl : T::first_type...
{
    static constexpr const Name baked_meta_name {n};  
    static constexpr const char* meta_name {n.cstr()};  
    static constexpr Members<typename T::second_type...> meta_members {};
};

#define Meta_Field(type, name) decltype([]{struct _M {type name;}; return Pair<_M, Reflectable<&_M::name, #name>>{};}())

#define Meta_Struct(x, ...) using x = Meta_Struct_Impl<#x, __VA_ARGS__>

Meta_Struct(V2, Meta_Field(float, x),
                Meta_Field(float, y));

using namespace std;

int main()
{
    tuple t {1, "2", '3'};
    tuple t2 {4, "5", '6'};
    int arr[3] {7, 8, 9};
    forc(int i = 0, i < 3, i + 1)
    {
        println("% % %", std::get<i>(t), std::get<i>(t2), arr[i]);
    };
}
