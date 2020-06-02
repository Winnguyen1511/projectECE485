# projectECE485 Cache simulation
Author: Nguyen Huynh Dang Khoa (aka Victor Nguyen), Nguyen Thi Minh Hien  

May-June 2020
Mayjor: Embedded System - Computer Science
University: Danang University of Sciences and Technology  
## About
- This project is about cache design and simulation. The goal is to design and similate 2 L1 caches: instruction cache and data cache which are backed by a shared L2 cache.  
![cache_image]()
- Instruction cache specification: 2-way associative, 16K sets, 64-byte line.
- Data cache specification: 4-way associative, 16K sets, 64-byte line.
- Both cache use LRU replacement policy, the order of cache is inclusive.

