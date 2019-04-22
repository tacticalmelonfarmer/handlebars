# When performance and memory are critical...
Sometimes you are less worried about simple frictionless usage and care more about predictable 
behaviour, performance and memory usage. For this case i have written a 
dispatcher ("include/handlebars/fast/dispatcher.hpp", `namespace handlebars::fast`) 
that only accpets signals types of enum class type. This enum class signal type must have a set of identifiers 
with the very first ordinal value being `0`, immediately after the identifier list must be an identifier called 
`enum_size` this is a bit of magic to tell how big the static handler map must be which is decided at compile-time. 
**DO NOT DEFINE YOUR OWN VALUES FOR ENUM SIGNAL IDENTIFERS, LEAVE AS DEFAULT STARTING AT 0**. At the end of the enum you can define your 
own configuration values: 
  + `ENUM::max_handlers_per_signal`:
    + defines how many handlers can be connected to a single signal type at any one time
  + `ENUM::max_events_enqueued`:
    + defines the internal array size for the event queue