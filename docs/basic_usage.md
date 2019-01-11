# How to handle events in Handlebars
An event handler in Handlebars is a *callable* taking arguments of whatever types are specified by the user, with `void` return type. 
They can be *connected*, *have arguments bound to them* or *be responded to*. The signal type and arguments types together are 
called the *event signature*.

**NOTE**: cv-qualified *pointers to member functions* are allowed, because the cv-qualifiers are type-erased away.

## Connecting Free functions and Static member functions
In C++, a pointer to function can be assigned the address of any free function, as well as **static** member functions. 
This is because static member functions are semantically the same as free functions. Anyhow, creating an event handler from 
a function pointer is simple. First you must decide what arguments the handler is going to recieve such as: inputs *(value, rvalue ref or const lvalue ref)* 
or ouputs *(pointer, reference...)*. Then define a *signal* type, for which the handler will respond, predicated on the value. for example:

```c++
enum class signals { add, subtract };

void add(int& output, int input)
{
    ouput += input;
}

struct math
{
    static void subtract(int& ouput, int input)
    {
        ouput -= input;
    }
};
```

Here we define the signal type and some functions which can be pointed to normally through a `void(*)(int&,int)`. So making them into event handlers takes just a few lines of code, making sure to include the proper header, use the `connect` function:

```c++

#include <handlebars/dispatcher.hpp>
using d = handlebars::dispatcher<int&, int>;

int main()
{
    d::connect(signals::add, &add);
    d::connect(signals::subtract, &math::subtract);
    ...
}
```

There you go, almost too easy. Next comes the more advanced topics...

## Conneting Non-static member functions
The part where things get a little more complicated is when you desire to use a member function which is not static. 
Because they are allowed access to a pointer called `this`, you must provide that pointer along with the address of the member function. 
Connection is made through the `connect_member` function:

```c++
using signals = int;

struct object
{
    void method(const string& msg);
    void operator()(const string& msg);
    void bind_me(int, const string&);
};
```

We've got the handler and the signal type set up. Now there are four different ways to do this, and it depends on how your handlers lifetime is managed. 
If you can guarantee that your handler won't be needed outside of it's defining scope, then you can pass the address:

```c++
int main()
{
    object o;
    d::connect_member(0, &o, &object::method);
    ...

```

However, you maybe used to using shared objects through `std::shared_ptr`. If so, then it is perfectly valid to pass a `std::shared_ptr` and it behaves as expected:


```c++
    ...
    std::shared_ptr<object> so(new object);
    d::connect_member(1, so, &object::method);
    ...

```

Now, we have the **functor/lambda** option, which only requires an instance object, by pointer or shared_ptr:

```c++
    ...
    auto l = [](auto msg){ print(msg); };
    d::connect(2, &l);
    d::connect(2, &o);
    d::connect(2, so);
}

```

Finally, there is the copy or move option, which uses the above interfaces, but takes instances by value instead. 
This option disconnects you from directly accessing the state of the handler instance, but keeps it valid for the life of the program.

```c++
    ...
    d::connect(3, [](auto msg){} );
    d::connect(3, o);
    d::connect(3, object{});
    ...
```

## Binding a function
Some design patterns require each event handler to provide some persistent data when being connected. The use of a *bind lambda*\* 
can help you roll your own solution, otherwise, you can use the `connect_bind` and `connect_bind_member` functions which are provided:

```c++
    ...
    auto bind_me = [](int id, const string& msg) { printf("%d: %s", i, msg); };
    d::connect_bind(4, bind_me, 0);
    d::connect_bind(4, bind_me, 1);
    d::connect_bind_member(4, &o, &object::bind_me, 2);
    d::connect_bind_member(4, so, &object::bind_me, 3);
    ...
```

The provided binding functions do not support placeholders, and binding happens from the left to right. All remaining 
arguments which have not been bound must match your event signature. 

\* a bind lambda is just a lambda wrapper that matches the event signature. Usually it is better to use these instead of `std::bind`.

## Pushing an event onto the queue
In order to tell the dispatcher that an event has happened we need to call `push_event` and provide arguments that match 
the event signature:

```c++
    ...
    d::push_event(0, "call pointer to member");
    d::push_event(1, "call shared pointer to member");
    d::push_event(2, "call pointed to lambdas and functors using operator()");
    d::push_event(3, "call internally stored lambdas and functors");
    ...
```

Now that we have pending events we can respond to them whenever we see fit.

## Responding to events
To the event handlers for events in the queue you will want to call the `respond` function. You can provide 
a limit for the amount of events you want handled. However, if you don't specify a limit it defaults to 0, 
which handles all currently pending events.

```c++
    d::respond(1); // responds to 1 event
    d::respond(0); // responds to all events; default is 0, so no need to provide it if responding to all events
}

```
