# Handlebars
This is a header only library, using C++17 features.

Handlebars provides mechanisms for having global event handlers of a single signature interface, although being a templated global interface you can handle as many signature types as you see fit.

### Dispatcher
the global interface is found in **include/handlebars/dispatcher.hpp**.
The interface is a single class with static member functions and static data. 
An event handler can be a free/static member function, 
a member function bound to an instance, a lambda or 
all of the above with extra bound arguments as 
if calling *std::bind*, but not. Accessing is easy just 
call a `handlebars::dispatcher<...>` method such 
as `connect` or `push_event`

### Handles
There is another interface found in 
**include/handlebars/handles.hpp** that hides the 
global interface. A class can derive from it. 
It uses the *crtp* to
make the syntax of adding a member function 
a bit easier to read. Your class must inherit `handlebars::handles<...>`
and provide its own type as the first template argument.
This interface has similar methods to `handlebars::dispatcher<...>`.

### Function
The dispatcher class uses a custom implementation of a callable wrapper
that explcictly does not allocate anything on the heap. To adjust
the fixed stack allocation size do `#define HANDLEBARS_FUNCTION_COMMON_MAX_SIZE`
to be a positive integer literal that you would prefer.

##### Note: be careful when using `conect_bind` or `connect_bind_member` methods, as the stored callable will take up more stack space

## Documentation
in the **docs** folder in the root of this repository, i wrote a little guide on how to use this library