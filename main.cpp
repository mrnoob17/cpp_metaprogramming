#include <cstdio>
#include <cassert>
#include <string>
#include <sstream>
#include <cstddef>
#include <vector>
#include <sstream>

using namespace std;

template<auto T>
struct constant
{
    static constexpr decltype(T) value {T};
    constexpr operator auto ()
    {
        return T;
    }
};

template<size_t I>
struct comp_time_loop
{
    constexpr comp_time_loop() {}

    template<typename L, typename Inc>
    struct holder
    {
        constexpr holder(L l, Inc ic) : c(l), inc(ic){}

        auto operator = (auto t)
        {
            t.template operator()<I>();
            constexpr auto iv {decltype(inc.template operator()<I>())::value};
            if constexpr(decltype(c.template operator()<iv>())::value){
                comp_time_loop<iv>{}(c, inc) = t;
            }
        }
        L c;
        Inc inc;
    };

    template<typename C, typename Inc>
    constexpr auto operator()(C c, Inc inc) 
    {
        return holder<C, Inc>{c, inc};
    }
};


// helper loop that has similar syntax with classic C loops
#define cfor(init, cond, inc) comp_time_loop<[&]<init>{return inc;}() - ([&]<init>(){return inc * 2;}() - [&]<init>(){return inc;}())>\
                                            {}([&]<init>{return constant<(cond)>{};}, [&]<init>{return constant<inc>{};}) = [&]<init>

template <size_t N>
struct baked_name
{
    constexpr baked_name(const char (&str)[N])
    { 
        for(int i = 0; i < N; i++){
            data[i] = str[i];
        }
    }

    template<auto U>
    constexpr bool operator == (const baked_name<U>& n) const
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
    template<auto U>
    constexpr bool operator != (const baked_name<U>& n) const
    {
        return (*this == n) == false;
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
struct pointer_to_member
{
    constexpr pointer_to_member(const T t){
        type = t;
    }

    template<typename U>
    constexpr bool operator == (const pointer_to_member<U>& m)
    {
        if constexpr(is_same_v<T, U>){
            return type == m.type;
        }
        return false;
    }

    T type;
};

template<pointer_to_member U, baked_name N, auto Mix_Type, auto Mix_Pointer_Type, auto Static_Array_Type>
struct reflectable
{
    constexpr reflectable () {}
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
    static constexpr const decltype(N) baked_name {N.cstr()};
};

// recursive macros from https://www.scs.stanford.edu/~dm/blog/va-opt.html

#define PARENS ()

#define EXPAND(...) EXPAND4(EXPAND4(EXPAND4(EXPAND4(__VA_ARGS__))))
#define EXPAND4(...) EXPAND3(EXPAND3(EXPAND3(EXPAND3(__VA_ARGS__))))
#define EXPAND3(...) EXPAND2(EXPAND2(EXPAND2(EXPAND2(__VA_ARGS__))))
#define EXPAND2(...) EXPAND1(EXPAND1(EXPAND1(EXPAND1(__VA_ARGS__))))
#define EXPAND1(...) __VA_ARGS__

#define FOR_EACH_MEMBER(...) __VA_OPT__(EXPAND(FOR_EACH_MEMBER_NEXT(__VA_ARGS__)))

#define FOR_EACH_MEMBER_NEXT(a, ...) reflectable<&meta_tag::a, #a,\
                                      [](){struct T {\
                                      decltype(pointer_to_member_type(&meta_tag::a)) a;}; return T{};},\
                                      [](){struct T {\
                                      decltype(pointer_to_member_type(&meta_tag::a))* a;}; return T{};},\
                                      []<auto I>(){struct T {\
                                      decltype(pointer_to_member_type(&meta_tag::a)) a[I];}; return T{};}>\
                                     __VA_OPT__(,) __VA_OPT__(FOR_EACH_MEMBER_NEXT2 PARENS (__VA_ARGS__))

#define FOR_EACH_MEMBER_NEXT2() FOR_EACH_MEMBER_NEXT 

#define Reflect(name, ...) using meta_tag = name; static constexpr reflect_data<meta_tag, #name, FOR_EACH_MEMBER(__VA_ARGS__)> meta{}

template<typename ...Ts>
consteval bool has_duplicate_members()
{
    constexpr const char* names[] {Ts{}.name...};
    constexpr auto count {sizeof...(Ts)};

    struct const_name
    {
        constexpr const_name(const char* str): data(str) {}

        constexpr bool operator == (const const_name str) const
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

    for(int i = 0; i < count; i++)
    {
        for(int j = 0; j < count; j++)
        {
            if(i != j)
            {
                if(const_name{names[i]} == const_name{names[j]}){
                    return true;
                }
            }
        }
    }
    return false;
}

template<typename ...Ts> 
struct reflect_members 
{
    constexpr reflect_members()
    {
        static_assert(!has_duplicate_members<Ts...>(), "has duplicate members");
    }

    static constexpr size_t count {sizeof...(Ts)};

    template<baked_name N, typename T, typename ...Us>
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

    template<baked_name N>
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
                return reflect_members<Us...>{}.template at<I, Current + 1>();
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

template<typename T, baked_name N, typename ...Ts>
struct reflect_data
{
    constexpr reflect_data() {}
    using type = T;
    static constexpr const char* name {N.cstr()};
    static constexpr reflect_members<Ts...> members;
};

struct RGBA
{
    float r {1};
    float g {3};
    float b {5};
    float a {7};

    string name {"skyblue"};

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
string string_builder(const auto& s)
{
    string result;
    if constexpr(requires{s.meta;})
    {
        result += '{';
        cfor(auto i = 0, i < s.meta.members.count, i + 1)
        {
            constexpr auto member {s.meta.members.template at<i>()};
            auto& v {member(s)};
            result += string_builder(v);
            if(i + 1 < s.meta.members.count){
                result += ", ";
            }
        };
        result += '}';
    }
    else // assume object can be converted to a string
    {
        //work around for strings
        if constexpr(requires {result == string{s};}){
            result += '"' + s + '"';
        }
        //work around for containers, can recurse here but let's be lazy
        else if constexpr(requires {s.begin();} && requires{s.end();}){
            result += "{...}";
        }
        // assume be POD 
        else{
            result += to_string(s);
        }
    }
    return result;
}

// naively prints a whole struct with name and names of members and their values
void pretty_print(auto& s)
{
    if constexpr(requires {s.meta;})
    {
        printf("struct %s {", s.meta.name);

        cfor(auto i = 0, i < s.meta.members.count, i + 1)
        {
            auto member {s.meta.members.template at<i>()};
            auto& v {member(s)};

            const auto vs {string_builder(v)};
            if(vs.empty()){
                printf("%s : {...}", member.name);
            }
            else{
                printf("%s : %s", member.name, vs.c_str());
            }

            if constexpr(i + 1 < s.meta.members.count){
                printf(", ");
            }
        };
        printf("}\n");
    }
    else{
        printf("%s", string_builder(s).c_str());
    }
}

void pretty_printnl(auto& s) requires requires {s.meta;}
{
    pretty_print(s);
    printf("\n");
}

void ugly_printnl(auto& s)
{
    printf("%s", string_builder(s).c_str());
    printf("\n");
}

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

template<reflect_members M>
struct widget_internal : widget
{
    static constexpr decltype(M) members {M};
    void show()
    {
        cfor(auto i = 0, i < members.count, i + 1)
        {
            auto m {members.template get<i>()};
            using type = decltype(m)::value_type;
            printf("member %s = %s\n", m.name, string_builder(*(type*)values[i]).c_str());
        };
    }
    void set(const string& s, const string& s2)
    {
        bool found {false};
        cfor(auto i = 0, i < members.count, i + 1)
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
    cfor(auto i = 0, i < e.meta.members.count, i + 1)
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
    return []<typename ...Ts>(const reflect_members<Ts...>&)
    {
        return soa<I, T, Ts...>{};
    }(T::meta.members);
};

auto widget_test()
{
    entity e;
    e.x = 69;
    e.y = 420;
    e.name = "deez nuts";
    widget* w {make_widget(e)};
    w->show();
    w->set("x", "69.420");
    w->show();
    delete w;
    dog d;
    w = make_widget(d);
    w->show();
    w->set("age", "5");
    w->show();
    delete w;
}

int main()
{
    auto s {new_soa<100, dog>()};  
    s.age[0] = 5;
    s.owner[0] = "drunkard steve";
    s.shelter_name[0] = "N/A";

    //auto s2 {new_soa<100, entity>()};  
    //s2.x[0] = 69;
    //s2.y[0] = 420;
    //s2.name[0] = "thing";

    entity e;
    e.x = 69;
    e.y = 420;
    e.name = "arnold";
    printf("%s\n", string_builder(e).c_str());

    dog d;
    d.age = 2;
    d.owner = "erika";
    d.shelter_name = "street dog sanctuary";

    pretty_print(d);
}
