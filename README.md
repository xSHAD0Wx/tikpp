# tikpp - An asynchronous modern C++ Mikrotik RouterOS API library

### Features
- An easy to use interface
- Making use of C++17 features (feature?)
- Fully asynchronous (Callbacks/Futures/Coroutines supported via Boost.Asio completion tokens)
- No RTTI is being used in the library (Templates!)
- No expcetions are being used

### Getting started
##### Creating an API connection:

First, you need to add an error handling function to your class/struct
```cpp
class my_app {
    ///...
  public:
    void on_error(const boost::system::error_code& err) {
        // handler error
    }
    ///...
};
```
Then create an ``` tikpp::io_context ``` (An alias of ```boost::asio::io_context```) instance and an ```tikpp::api``` object. This object will handle the asynchronous event loop of the library.
```cpp
#include "tikpp/io_context.hpp"
#include "tikpp/api.hpp"

///...

tikpp::io_context io{};
auto api = api = tikpp::api<my_app>::create(io, app_object);
```
Then start the ```tikpp::io_context``` object by calling ```io.run() ``` . **Remember** that the call to ```io.run() ``` will block the calling thread, so you must call it after initiating at least one asynchronous operation, or in another thread.

## Examples
### Sending a request

First, you need to create a request object
```cpp
auto req = api->make_request("/path/to/command");
```
Then, add parameters and attributes to the request object
```cpp
req->add_param("foo", "value");
req->add_attribute("bar", "value");
```
{fmt}/Python formats can be used to specify values
```cpp
req->add_param("param-key", "Hello, {}{}", "World", '!');
```
Finally, send the request

```cpp
// Sending a request and handling the response with a callback
api->async_send(std::move(req), [](const auto& err, auto&& resp) {
    // Check if the request succeeded
    if (err) {
        // handle error
    }
    
    // Check response type
    if (resp.type == tikpp::response_type::...) {
        // Do something
    }
    
    // Get response fields
    const auto& value1 = resp["resp-key-1"];
    const auto& value2 = resp["resp-key-2"];
    // ...
    
    // Return false to remove this handler from handlers map
    // If true is returned, this handler will remain in the completion handlers map (Used for
    // commands that return multiple responses, such as: getall, listen, etc...)
    return false;
});
```
Futures can be used to handle the response (Note that futures are generally slower, and will throw an expcetion if the request fails)
```cpp
#include "tikpp/tokens.hpp"
auto retf = api->async_send(std::move(api), tikpp::use_future);
//
// Do some work...
//

try {
    auto value = retf.get();
} catch (const boost::system::system_error& e) {
    // handle error
}
```
### Using the data repository
You can use a higher level repository-pattern object to manage data of the router.

Create a repository with a data model type as a template parameter
```cpp
// Include the data model header 
#include "tikpp/data/ip/hotspot/user.hpp"
#include "tikpp/data/repository.hpp"
///...
// Create a repository object with the API object
auto repo = tikpp::data::make_repository<tikpp::data::ip::hotspot::user>(api);
```
Load object list
```cpp
// Using completion callbacks
repo.async_load([](const auto& err, auto&& users) {
   if (err) {
       // handler errors
   }
   
   // Use users
   for (const auto& user : user) {
       // ...
   }
});

// Using futures
auto users = repo.async_load(tikpp::use_futures);

// Use users
for (const auto& user : user) {
    // ...
}
```
Queries can be used to filter the results
```cpp
// To use queries we need to create query tokens with the name of the query fields
auto [foo, bar] = tikpp::data::make_tokens("foo", "bar");
// Or
auto foo = "foo"_t, bar = "bar"_t;

repo.async_load(foo == "some_text" && bar <= 0xABCD, [](const auto& err, auto&& users) {
    // ...
});

// The query will be evaulated to
// ?=foo=some_text
// ?<bar=43981
// ?#!
// ?#&
// and appended to the request
```
Supported query operators are:  !, &&, ||, ^,==,!=,<,<=,>,>=

Objects can be loaded one at time using ```async_stream ```
```cpp
repo.async_stream([](const auto& err, auto&& user) {
   // ... 
});

// Queries can also be used here
repo.async_stream("foo"_t == "some_text" ...);
```
Add objects
```cpp
tikpp::data::ip::hotspot::user u{};
u.name = "name";
u.password = "pa$$w0rd";
u.limit_uptime = 10h;
u.limit_bytes_out = 10_gb;
//...
repo.async_add(std::move(u), [](const auto& err) { /* ... */ });
```
Remove objects
```cpp
repo.async_remove("*ABCD", [](const auto& err) { /* ... */ });
// Or
repo.async_remove(0xABCD, [](const auto& err) { /* ... */ });
```
Update objects
```cpp
// Get an object to be updated
auto user = repo.async_load("name"_t == "foo", tikpp::use_futures).get()[0];

// Update properties
// Only changed read_write properties will be sent back to the router
user.profile = "other-profile";
user.limit_bytes_out += 1_gb;
// ...

repo.async_update(user, [](const auto& err) { /* ... */ });
```
Sometimes you only want some of the data model properties (mostly for efficiency reasons). You can add your own types.
Only the specified properties will be used when interacting the router
```cpp
struct my_model : tikpp::data::model {
    read_only<std::string>                read_only_data;
    read_write<tikpp::data::types::bytes> bytes_data;
    read_write<std::uint32_t>             other_data;
    
    template<typename Converter>
    void convert(Converter& c) {
        // Call base convert function
        model::convert(c);
        
        c["read-only-data"] % read_only_data;
        c["bytes-data"]     % bytes_data;
        c["other-data"]     % other_data;
    }
};
```
