#include <cstdio>
#include <cstring>
#include <cassert>

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

        for(auto i = 0; i < len; i++)
        {
            if(n[i] != data[i]){
                return false;
            }
        }

        return true;
    }
    constexpr bool operator == (const char* str) const
    {
        for(auto c = str; *c != 0; c++)
        {
            if(*c != data[c - str]){
                return false;
            }
        }
        return true;
    }
    constexpr const char* cstr() const
    {
        return data;
    }

    static constexpr size_t len {N};
    char data[N];
};

template<typename T>
struct Pointer_To_Member
{
    using Value_Type = T;
    constexpr Pointer_To_Member(const T t){
        type = t;
    }
    T type;
};

template<Pointer_To_Member U, Name n>
struct Reflectable
{
    template<typename T>
    constexpr auto& operator()(T& t)
    {
        return t.*type;
    }

    static constexpr const decltype(U.type) type {U.type};
    static constexpr const char* name {n.cstr()};
};

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
                for(auto a = names[i]; *a != 0; a++)
                {
                    auto not_equal {false};
                    for(auto b = names[j]; *b != 0; b++)
                    {
                        if(*a != *b)
                        {
                            not_equal = true;
                            break;
                        }
                    }
                    if(!not_equal){
                        return false;
                    }
                }
            }
        }
    }

    return true;
}

template<typename ...Ts> 
struct Members 
{
    static constexpr const char* names[sizeof...(Ts)] {Ts{}.name...};

    static_assert(no_duplicate_members<Ts...>(), "error when creating members meta data: list has duplicate members");

    static constexpr size_t length {sizeof...(Ts)};

    template<typename T, typename F>
    constexpr void operator()(T& t, F f) const
    {
        (f(t, Ts{}), ...);
    }

    template<Name N, typename T, typename ...Us>
    constexpr auto by_name_helper() const
    {
        if constexpr (N == T{}.name){
            return T{}.type;
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
    constexpr auto member_at() const
    {
        static_assert(Current <= I, "member_at error : index out of bounds");
        return []<typename T, typename ...Us>(T t, Us... us)
        {
            if constexpr(Current != I){
                return Members<Us...>{}.template member_at<I, Current + 1>();
            }
            else{
                return T{};
            }

        }(Ts{}...);
    }

    template <Name N, typename T>
    constexpr auto& value(T& t) const
    {
        return t.*by_name<N, Ts...>();
    }

};

#include <string>

constexpr auto& reflective_get(auto& s, auto& t)
{
    return s.*t.type;
}

template<size_t index = 0>
constexpr auto for_each(auto& s, auto t)
{
    constexpr auto v {s.meta.members.template member_at<index>()};
    t(s, v);

    if constexpr(index + 1 < s.meta.members.length){
        for_each<index + 1>(s, t);
    }
}

template<typename T, Name N, typename ...Ts>
struct Reflect_Data
{
    using type = T;
    static constexpr const char* name {N.cstr()};
    Members<Ts...> members;
};

#define Reflect(name, ...) using meta_tag = name; static constexpr Reflect_Data<meta_tag, #name, __VA_ARGS__> meta{}

#define Field(a) Reflectable<&meta_tag::a, #a>

struct RGBA
{
    float r {1};
    float g {3};
    float b {5};
    float a {7};

    Reflect(RGBA, Field(r), Field(g), Field(b), Field(a));
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

template<size_t I = 0, bool Nested = false>
void print(auto& s) requires requires {s.meta.members;}
{
    constexpr auto member {s.meta.members.template member_at<I>()};
    auto& v {reflective_get(s, member)};

    if constexpr(!Nested && I == 0){
        printf("struct %s {", s.meta.name);
    }
    if constexpr(requires{v.meta.members;})
    {
        printf("%s : {", member.name);
        print<0, true>(v);
        if constexpr(I + 1 < s.meta.members.length)
        {
            printf("}\n");
            print<I + 1>(s);
        }
        else{
            printf("}");
        }
    }
    else
    {
        if constexpr(I + 1 >= s.meta.members.length){
            printf("%s : %s", member.name, std::to_string(v).c_str());
        }
        else
        {
            if constexpr(Nested){
                printf("%s : %s, ", member.name, std::to_string(v).c_str());
            } 
            else{
                printf("%s : %s,\n", member.name, std::to_string(v).c_str());
            } 
            print<I + 1, Nested>(s);
        }
    }
    if constexpr(!Nested && I == 0){
        printf("}");
    }
}

void println(auto& s) requires requires {s.meta.members;}
{
    print(s);
    printf("\n");
}

int main()
{
    Foo f;

    f.bar = 5;
    f.zed = 1;
    f.position = {1, 2};
    f.heading = {6, 9};
    f.origin = {4, 20};

    constexpr auto position {Foo::meta.members.by_name<"position">()};

    for_each(f, [](auto& s, auto i)
    {
        auto& v {i(s)};
        if constexpr(requires{v + 1;}){
            printf("member %s has + operator\n", i.name);
        }
    });

    println(f);
}
