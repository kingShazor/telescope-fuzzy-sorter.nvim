# telescope-fuzzy-sorter.nvim

fuzzy-sorter is a custom sorter for Telescope. It will be used to find files. My goal was to create a lightweight and fast sorter which is easy to maintain.

## Features

The input query is split into individual search words. Each search word is scored separately. There are two automatically applied modes: a fuzzy mode and a strict mode. 
Strict mode is triggered for search words containing uppercase or non-ASCII characters.
It performs a case-sensitive, continuous substring search using the Boyer–Moore–Horspool algorithm.
The fuzzy mode is case-insensitive and allows small gaps, such fill characters will not break the match.

### fuzzy-examples

"locationfind" finds the file "location_finder.cpp".  
"location" finds the file "LOCATION.md"  

### strict-examples

"LOCATION" finds "LOATION.md"  
"Location" doesn't find "LOC"  

### no full UTF8-Support - non-ascii treated as strict search words

"übertrieben" does find "übertrieben.md"  
"übertrieben" doesn't find "Übertrieben.md"  

### more keyword/mixing both modes

"loc sear" finds "geo/location_search.c"  
"Loca sear" find "geo/Location_search.c" but not "geo/location_search"  

### word parts can be used only once

"ue que" finds "network_queue.h" but not "unique_ptr.h"  
"ue que cpp" finds network_queue.cpp" but not "network_queue.h"  

### the order of the key words doesn't matter

"location util" finds "location_util.cpp"  
"util location" finds "location_util.cpp"  

## Limitations
The sorter doesn't support regular expression, inverse search, prefix or suffix search.  

