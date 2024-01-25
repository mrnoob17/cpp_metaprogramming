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
    static constexpr size_t len {N};
    char data[N];
};

template<typename T>
struct Pointer
{
    constexpr Pointer(const T t){
        type = t;
    }
    T type;
};

template<Pointer U, Name n>
struct Reflectable
{
    constexpr auto type() const
    {
        return U.type;
    }

    constexpr const char* name() const
    {
        return n.data;
    }
};

struct Serializer
{
    template<typename T>
    void operator()(const char* f, T& t) const
    {
    }
};

#define Reflect(b) Reflectable<&Struct_Type::b, #b>
    
template<typename ...Ts>
struct Members
{
    static constexpr const char* names[sizeof...(Ts)] {Ts{}.name()...};
    static constexpr size_t length {sizeof...(Ts)};

    template<typename T, typename F>
    constexpr void operator()(T& t, F f) const
    {
        (f(t, Ts{}), ...);
    }

    template<Name N, typename T, typename ...Us>
    constexpr auto by_name() const
    {
        if constexpr (N == T{}.name()){
            return T{}.type();
        }
        else
        {
            return [this]<typename First, typename... Rest>(First f, Rest... r)
            {
                return by_name<N, Rest...>();
            }(Ts{}...);
        }
    }

    template<size_t I, size_t Counter = 0>
    constexpr auto member_at() const
    {
        return []<typename T, typename ...Us>(T t, Us... us)
        {
            if constexpr(Counter != I){
                return Members<Us...>{}.template member_at<I, Counter + 1>();
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


auto& get(auto& s, auto& t)
{
    return s.*t.type();
}

auto& for_each(auto& s, auto& t)
{
    s.members(s, t);
}

struct RGBA
{
    static constexpr const char* name {"RGBA"};

    float r {1};
    float g {3};
    float b {5};
    float a {7};


    using Struct_Type = RGBA; 
    static constexpr Members<Reflect(r),
                             Reflect(g),
                             Reflect(b),
                             Reflect(a)> members;
};

struct Vec
{
    static constexpr const char* name {"Vec"};

    float x {0};
    float y {0};

    using Struct_Type = Vec; 
    static constexpr Members<Reflect(x),
                             Reflect(y)> members;
};

struct Foo
{
    static constexpr const char* name {"Foo"};

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

    using Struct_Type = Foo; 
    static constexpr Members<Reflect(bar),
                             Reflect(zed),
                             Reflect(background),
                             Reflect(highlight),
                             Reflect(heading),
                             Reflect(origin),
                             Reflect(position)> members;
};

template<size_t I = 0, bool Nested = false>
void print(auto& s) requires requires {s.members;}
{
    constexpr auto member {s.members.template member_at<I>()};
    auto& v {get(s, member)};

    if constexpr(!Nested && I == 0){
        printf("struct %s {", s.name);
    }
    if constexpr(requires{v.members;})
    {
        printf("%s : {", member.name());
        print<0, true>(v);
        if constexpr(I + 1 < s.members.length)
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
        if constexpr(I + 1 >= s.members.length){
            printf("%s : %s", member.name(), std::to_string(v).c_str());
        }
        else
        {
            if constexpr(Nested){
                printf("%s : %s, ", member.name(), std::to_string(v).c_str());
            } 
            else{
                printf("%s : %s,\n", member.name(), std::to_string(v).c_str());
            } 
            print<I + 1, Nested>(s);
        }
    }
    if constexpr(!Nested && I == 0){
        printf("}");
    }
}

void println(auto& s) requires requires {s.members;}
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
    f.origin= {4, 20};

    println(f);
}
