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
    using value_type = decltype(pointer_to_member_type(U.type));
    static constexpr const decltype(U.type) type {U.type};
    static constexpr const char* name {N.cstr()};
    static constexpr const Name_Const_Char cc_name {N.cstr()};
};

#define Reflect(name, ...) using meta_tag = name; static constexpr Reflect_Data<meta_tag, #name, __VA_ARGS__> meta{}

#define Field(a) Reflectable<&meta_tag::a, #a>

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
    constexpr auto by_index() const
    {
        static_assert(Current <= I, "by_index error : index out of bounds");
        return []<typename T, typename ...Us>(T t, Us... us)
        {
            if constexpr(Current != I){
                return Members<Us...>{}.template by_index<I, Current + 1>();
            }
            else{
                return T{};
            }

        }(Ts{}...);
    }

    template<size_t I>
    constexpr auto get() const
    {
        return by_index<I>(); 
    }
};

template<size_t index = 0>
constexpr auto for_each(auto& s, auto t)
{
    constexpr auto v {s.meta.members.template by_index<index>()};
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

    std::string name {"nameless color"};

    Reflect(RGBA, Field(r), Field(g), Field(b), Field(a), Field(name));
};

struct Vec
{
    float x {0};
    float y {0};

    Reflect(Vec, Field(x), Field(y));
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

    Reflect(Foo, Field(bar), Field(zed), Field(position), Field(heading), Field(origin), Field(background), Field(highlight));
};

// naively converts an object to a string
// can be good for serialization
template<size_t I = 0, bool Nested = false>
std::string to_string(const auto& s)
{
    std::string result;
    if constexpr(requires{s.meta.members;})
    {
        constexpr auto member {s.meta.members.template by_index<I>()};
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

        constexpr auto member {s.meta.members.template by_index<I>()};
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

    constexpr auto member {s.meta.members.template by_index<I>()};
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
    static constexpr const From  base {From{}};
    static constexpr const char* name {T{}.name};

    T::value_type data[S]{};

    template<Name N>
    constexpr auto field() const
    {
        if constexpr(name == N)
        {
            struct View
            {
                constexpr View(const T::value_type* data){
                    view = data;
                }
                constexpr auto& operator[](size_t i) const
                {
                    assert(i < S);
                    return view[i];
                }
                constexpr auto begin() const{
                    return view;
                }
                constexpr auto end() const{
                    return view + S;
                }
                constexpr auto raw() const{
                    return view;
                }
                const T::value_type* view {nullptr};
            };
            return View{data};
        }
        else
        {
            const auto i {static_cast<const Parent*>(this)};
            return i->template field<N>();
        }
    }

    template<Name N>
    constexpr auto field()
    {
        if constexpr(name == N)
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
        else
        {
            auto i {static_cast<Parent*>(this)};
            return i->template field<N>();
        }
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
            return new_soa<S, From, N, SOA_Base<T, decltype(Members<Ts...>{}.template by_index<0>()), S, From>, Rest...>(Reflect_Data<From, N, Rest...>{});
        }(Ts{}...);
    }
}

struct Test
{
    int x;
    int y;
    int gold;
    int health;

    Reflect(Test, Field(x), Field(y), Field(gold), Field(health));
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
        // creating new structs
        // creating a soa array from the members
        // kinda weird way to access the fields though

        auto test_soa {new_soa<10>(Test::meta)};

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

        printf("\nsoa of %s member names are : ", test_soa.base.meta.name);
        for(auto m : test_soa.base.meta.members.names){
            printf("%s ", m);
        }

        printf("\n");

        {
            auto x {test_soa.field<"x">()};
            auto y {test_soa.field<"y">()};
            auto gold {test_soa.field<"gold">()};
            auto health {test_soa.field<"health">()};
            for(auto& g : gold){
                g = 100;
            }
        }
    }
}

#define _if(x) if constexpr(x)
#define _else_if(x) else if constexpr(x)

template<typename T, T ...>
struct Sequence
{
};

template<size_t start, size_t end>
struct Range
{
    template<auto L = start, auto ...P>
    constexpr auto make_sequence() const
    {
        _if(L != end){
            return make_sequence<L + 1, P..., L>();
        }
        else{
            return Sequence<size_t, P..., L>{};
        }
    }
};

template<auto T>
struct Constant
{
    constexpr operator decltype(T)()
    {
        return T;
    }
};

template<size_t start, size_t end>
struct CT_Loop
{
    constexpr CT_Loop(){}

    template<typename T>
    constexpr void operator = (T t) 
    {
        t.template operator()<start>();
        _if(start + 1 < end){
            CT_Loop<start + 1, end>{} = t;
        }
    }
};

#define _for(y, s, e) CT_Loop<s, e>{} = [&]<y>() 

void ct_test()
{
    Foo foo;

    _for(auto e, 0, 10){
        printf("%llu", e);
    };
}

int main()
{
    ct_test();
}
