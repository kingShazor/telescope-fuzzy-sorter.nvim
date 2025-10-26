# telescope-fuzzy-sorter.nvim

fuzzy-sorter is a custom sorter for Telescope. It will be used to find files. My goal was to create a lightweight and fast sorter which is easy to maintain.

## Features

The input query is split into individual search words. Each search word is scored separately. There are two automatically applied modes: a fuzzy mode and a strict mode. 
Strict mode is triggered for search words containing uppercase or non-ASCII characters.
It performs a case-sensitive, continuous substring search.
The fuzzy mode is case-insensitive and allows small gaps, such fill characters will not break the match.

#### fuzzy-examples

"locationfind" finds the file "location_finder.cpp".  
"location" finds the file "LOCATION.md"  

#### strict-examples

"LOCATION" finds "LOATION.md"  
"Location" doesn't find "LOC"  

#### more keyword/mixing both modes

"loc sear" finds "geo/location_search.c"  
"Loca sear" find "geo/Location_search.c" but not "geo/location_search"  

#### word parts can be used only once

"ue que" finds "network_queue.h" but not "unique_ptr.h"  
"ue que cpp" finds network_queue.cpp" but not "network_queue.h"  

#### the order of the key words doesn't matter

"location util" finds "location_util.cpp"  
"util location" finds "location_util.cpp"  

## Limitations

The sorter doesn't support regular expression, inverse search, prefix or suffix search. Mac and Windows Support is still experimental (not tested).

#### no full UTF-8-Support - non-ascii treated as strict search words

"übertrieben" does find "übertrieben.md"  
"übertrieben" doesn't find "Übertrieben.md"  

## Installation

#### Lazy

```lua
   --as dependency of telescope
    dependencies = {
      {
        'kingShazor/telescope-fuzzy-sorter.nvim',
        build = 'make',
        cond = function()
          return vim.fn.executable 'make' == 1
        end,
      },
```
#### register extension
```lua
-- load_extension, somewhere after setup function:
      pcall(require('telescope').load_extension, 'fuzzy_sorter')
```

## 🚀 Backlog

#### high prio

- [ ] lua opt: user can switch off global/files sorter
- [ ] lua opt: understand that prefilters
- [x] c++: write a usefull performance test (let chatgpt generate some names)
- [x] c++: optimize fuzzy - if pattern can't be found at pattern index p > 1, we probably don't need to start at i + 1 again
- [ ] c++/lua return/receive floats
- [ ] c++ use c string instead of string_view for text
- [ ] README.md add performance pros
- [ ] README.md add credits

#### mid prio

- [ ] README.md better installation guide
- [ ] README.md add installation guide for devs
- [ ] README.md add small presentation vid
- [ ] full Utf8-Support (configurable)
- [ ] cmake build option for users
- [ ] ci
- [ ] Knuth-Morris-Pratt for strict patterns


#### low prio / more a 100+ users thing

- [ ] tests/support for mac
- [ ] tests/support for windows
- [ ] doxygen
- [ ] version-tag
- [ ] explicit filter for word ending '.cpp'
- [ ] filter for multiple file types '.cpp,h'
