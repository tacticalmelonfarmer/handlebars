# Handlebars
This is a header only library, using C++17 features.

Handlebars provides mechanisms for having global event handlers of a single signature interface, although being a templated global interface you can handle as many signature types as you see fit.

### Dispatcher
the global interface is found in *include/handlebars/dispatcher.hpp*.
The interface is a single class with static member functions and static data. 
An event handler can be a free/static member function, 
a member function bound to an instance, a lambda or 
all of the above with extra bound arguments as 
if calling *std::bind*. Accessing is easy just 
call a `handlebars::dispatcher<...>` method such 
as `connect` or `push_event`
see example found in *example/global_api*

### Handler
There is another interface found in 
*include/handlebars/handler.hpp* that hides the 
global interface. A class can derive from it. 
It uses the *crtp* pattern to
make the syntax of adding a member function 
a bit easier to read. Your class must inherit `handlebars::handler<...>`
and provide its own type as the first template argument.
This interface has similar methods to `handlebars::dispatcher<...>`.
see example found in *example/minimal_api*

### Function
The dispatcher class uses a custom implementation of a function holder
that explcictly does not allocate anything on the heap. To adjust
the fixed stack allocation size do `#define HANDLEBARS_FUNCTION_COMMON_MAX_SIZE`
to be a positive integer literal that you would prefer.
see example in *example/function* for all the different ways you can construct a function.

##### Note: be careful when using `conect_bind` or `connect_bind_member` methods, as the stored callable will take up more stack space