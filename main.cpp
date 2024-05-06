#include <cstdio>
#include <cassert>
#include <string>
#include <sstream>
#include <vector>
#include <cstddef>

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
U pointer_to_member_type(U C::*v);

template <class C, typename U>
C pointer_to_member_base(U C::*v);

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

template<Pointer_To_Member U, Name N, auto Mix_Type, auto Mix_Pointer_Type, auto Static_Array_Type>
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
    using base = decltype(pointer_to_member_base(U.type));
    using value_type = decltype(pointer_to_member_type(U.type));
    using value_mix_in = decltype(Mix_Type());
    using value_pointer_mix_in = decltype(Mix_Pointer_Type());
    template<auto I>
    using static_array_mix_in = decltype(Static_Array_Type.template operator()<I>());
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

#define FOR_EACH_MEMBER_NEXT(a, ...) Reflectable<&meta_tag::a, #a,\
                                      [](){struct T {\
                                      decltype(pointer_to_member_type(&meta_tag::a)) a;}; return T{};},\
                                      [](){struct T {\
                                      decltype(pointer_to_member_type(&meta_tag::a))* a;}; return T{};},\
                                      []<auto I>(){struct T {\
                                      decltype(pointer_to_member_type(&meta_tag::a)) a[I];}; return T{};}>\
                                     __VA_OPT__(,) __VA_OPT__(FOR_EACH_MEMBER_NEXT2 PARENS (__VA_ARGS__))

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

#define _if(x) if constexpr(x)
#define _else_if(x) else if constexpr(x)

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
            _if(requires {t.template operator()<I>() == Exit<true>{};}){
                return t.template operator()<I>();
            }
            else{
                t.template operator()<I>();
            }
            constexpr auto iv {decltype(inc.template operator()<I>())::value};
            _if(decltype(c.template operator()<iv>())::value){
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
        _if(nl){
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

#include <type_traits>

template<auto S, auto E>
struct sequence;

template<auto E>
struct sequence<E, E>
{
    static constexpr bool iter {true};
};

template<auto S, auto E, auto C = 0>
struct value_getter
{
    constexpr value_getter() {}
    constexpr auto get(const int n)
    {
        if(C == n)
        {
            constexpr auto k {C};
            return k;
        }
        else{
            return value_getter<S, E, C + 1>{}.get(n);
        }
    }
};

template<auto S, auto E>
struct value_getter<S, E, 128>
{
    constexpr value_getter() {}
    constexpr auto get(const int n)
    {
        return 128;
    }
};

template<typename T>
concept baked = requires(T t){t.value;};

template<auto S, auto E>
struct sequence : sequence<S + 1, E>
{
    static constexpr bool iter {false};
    static constexpr decltype(S) value {S};
    using next = sequence<S + 1, E>;
    struct iterator
    {
        decltype(S) current;
        constexpr iterator(auto n) : current(n)
        {
        }
        constexpr auto operator*()
        {
            //if constexpr(C == S)
            //{
            //    constexpr Constant<C> it;
            //    return it;
            //}
            //else if constexpr(C < E){
            //    return this->operator*<C + 1>();
            //}
        }
        //template<>
        //constexpr auto operator*<128>()
        //{
        //    constexpr Constant<128> it;
        //    return it;
        //}
        template<auto I = S>
        void operator++()
        {
            return this->operator++<I + 1>();
        }
        template<>
        void operator++<E>()
        {
            return;
        }
        constexpr bool operator == (const auto m){
            return false;
        }
        sequence<S, E> pointer;
    };
    constexpr auto begin()
    {
        return iterator{S};
    }
    constexpr auto end()
    {
        return iterator{E};
    }
};

struct foo
{
    static constexpr int const arr []{1, 2, 3, 4, 5};
    template<auto counter>
    struct iterator
    {
        consteval int operator* ()
        {
            int p;
            counter(&p);
            return p;
        }
        constexpr void operator++()
        {
        }
    };
    constexpr auto begin()
    {
        return iterator<[]<auto c = 1>(int* p) consteval {
            if(p){
                *p = c;
            }
            return c;
        }>{};
    }
    constexpr auto end()
    {
        return iterator<[]<auto c = 1>(int* p) consteval {
            if(p){
                *p = c;
            }
            return c;
        }>{};
    }
};

template<size_t I>
struct tuple_index{
    static constexpr size_t value {I};
};

template<typename ...Ts>
struct tuple;

template<>
struct tuple<>{};

template<typename T, typename ...Ts>
struct tuple<T, Ts...> : tuple<Ts...>
{
    constexpr tuple(const T& t, const Ts& ...ts)
    {
        value = t;
        ((tuple<Ts...>::value = ts),...);
    }

    constexpr tuple(){}

    T value {};

    template<size_t I, size_t C = 0>
    constexpr auto& get() const
    {
        if constexpr(C == I){
            return value;
        }
        else
        {
            auto c {static_cast<const tuple<Ts...>*>(this)};
            return c->template get<I, C + 1>();
        }
    };

    template<size_t I, size_t C = 0>
    constexpr auto pointer() const
    {
        if constexpr(C == I)
        {
            const void* c {&value};
            return static_cast<const char*>(c);
        }
        else
        {
            auto c {static_cast<const tuple<Ts...>*>(this)};
            return c->template get<I, C + 1>();
        }
    };
};

template<typename T, typename ...Ts>
tuple(T, Ts...) -> tuple<T, Ts...>;

#include <string>
#include <sstream>

using namespace std;

struct entity
{
    float x;
    float y;

    string name;

    Reflect(entity, x, y, name);
};

struct dog
{
    int age {0};
    string owner {"tkap"};
    string shelter_name {"deez nuts sanctuary"};
    Reflect(dog, age, owner, shelter_name);
};

struct widget
{
    vector<void*> values;
    virtual void show() = 0;
    virtual void set(const string&, const string&) = 0;
    virtual ~widget() = default;
};

template<Members M>
struct widget_internal : widget
{
    static constexpr decltype(M) members {M};
    void show()
    {
        forc(auto i = 0, i < members.count, i + 1)
        {
            auto m {members.template get<i>()};
            using type = decltype(m)::value_type;
            printf("member %s = %s\n", m.name, to_string(*(type*)values[i]).c_str());
        };
    }
    void set(const string& s, const string& s2)
    {
        bool found {false};
        forc(auto i = 0, i < members.count, i + 1)
        {
            auto m {members.template get<i>()};
            if(m.name == s)
            {
                using type = decltype(m)::value_type;
                stringstream ss {s2};
                ss>>*(type*)values[i];
                found = true;
            }
        };
        if(!found){
            assert(false);
        }
    }
};

auto make_widget(auto& e)
{
    auto w {new widget_internal<e.meta.members>{}};
    forc(auto i = 0, i < e.meta.members.count, i + 1)
    {
        auto m {e.meta.members.template get<i>()};
        w->values.push_back(&m(e));
    };
    return w;
};

template<auto I, typename B, typename ...Ts>
struct soa : Ts::template static_array_mix_in<I>...
{
    using base = B;
    static constexpr decltype(I) count {I};
    constexpr soa(){}
};

template<size_t I, typename T> requires requires {T::meta;}
auto new_soa()
{
    return []<typename ...Ts>(const Members<Ts...>&)
    {
        return soa<I, T, Ts...>{};
    }(T::meta.members);
};

int main()
{
    auto s {new_soa<100, dog>()};  
    s.age[0] = 5;

    auto s2 {new_soa<100, entity>()};  
    s2.x[0] = 5;

    //entity e;
    //e.x = 69;
    //e.y = 420;
    //e.name = "deez nuts";
    //widget* w {make_widget(e)};
    //w->show();
    //w->set("x", "69.420");
    //w->show();
    //delete w;
    //dog d;
    //w = make_widget(d);
    //w->show();
    //w->set("age", "5");
    //w->show();
    //delete w;
}
