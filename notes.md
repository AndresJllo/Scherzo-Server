# notes on the project

## 2020-10-10
- ive decided that the best choice for the fstrie is to use a heap allocated thing for the moment. one, because ive already done it, two because it is more flexible to space allocation. this contrasts with my decision to use szcue in the fshandler. i used szdcue because i dont expect the server to load up many buckets at once, but strictly speaking, using a sized cue means that could overflow the stack. 

- notice that the fstrie could be optimized in a similar way to fshandler to only use the stack. you COULD make a szdtrie, only that you'd end up occupying `2 * 100 * 2^16 -1` bytes off the bat (and youre already using `2 * 2^16 -1` as it is. 

- that is to say, you could do a vector based trie, where a vector contains nodes and data for each one and there is an adjacency list. this should work well for the `add_str` and `has_str` operations. when faced with a delete operation, one could use a sized queue to handle avaible indices like in fshandler. however the szdqueue would take up a lot of space

- you could also solve this by using a normal std::queue. if i have time, ill try it out.

- the request handler should have some sort of "fshandler priority quee", where the most requested fshandlers are deallocated last, but inevitably so regarless. maybe the queue can move after each operation. 

## 2020-10-11
- ive decided that filenames can contain upper and lowercase letters, numbers, and the simbols `., -, and _`. this so that the trie works.

- i'm concerned about sized queue. i dont know what would happen if it started operating on the highest capacity. not gonna find out now.

- just a thought: maybe returning the `FILE*` to the request handler isnt such a good idea. on one hand, it is possible that the request handler `close()`'s the file, which would put the handler on an invalid state. further more, the filehandler would have no info on wether or not it's pointer is valid. i think returning the fileinfo is safer, but just as a wya to send info to the client, since you cant rely on it being valid (another thread or request might change the fs before you use it).

- just a thought: you've considered sending the fileinfo to the client, but as i said above this would eb purely symbolic to please juan carlos. in reality the finfo mightve changed for a file after the client did another requerst.

- i changed open to be just an array. some people say that it's wasting space, but honestly its much more manageable than a vector, it should also have slightly better allocation times and its not that heavy, its like 120kb.

- the size of a fshandler instance, at most, should be `2 * 2^16 (open) + 2 * 2^16 (avail) + 2^16 * MAX_NAME_LEN (max nodes) * (66 + 1 + 4 + 2)` = 4gb with a max length of 100 chars per filename. 

## 2020-10-12
- just a thought: #1, you considered makign it so fshandler operates purely on `file_id's`, in fact, you can just add those functions. however, it doesn't matter how you store the `file_id`, the fact is that it's going to be limited. and the fact that its going to be limited means that id's will get reassigned if you run out of them. and that means that if the client recieves a `file_id`, it might refer to a completely different file. this is true of names too, ofc, another client COULD write to the file before the first one does, and the file would be a different file, bu both of those people were originally thinking of the same file, whereas in the second case, the client's `file_id` might be reassigned by the time it gets to the server.


- thought #2: the most optimal way to implement that trie would be to make it with a vector of nodes, a queue of available `node_id, and a vector of vectors of `node_id` (and adjacency list). in this model, all operations would remain of the same complexity, except that now dorpping the tree is cheaper. now the vectors need to be heap allocated, cuz there can be too many nodes. but you can write a vector that takes advantage of the memset word trick when copying data, sinc ethese vectors are only gonna hold data of 16 and 32 bits (maybe 8). you can also implement a queue with this vector. 

- thought #3: a space-saving fshandler would have a map instead of a trie. consider implementing that version if you've time left over. not sure if to use dynamic dispatch to handle different structs, or if more simple make C-style file-handler structs that have functions that take in a pointer to a struct and determines with a flag if its space saving or not.

- on that note, consider implementing everything C-style, honestly.

- DONE: implement inline'd function to convert from file_id to num

- DONE: change fshandler function names to [operation]_fs 

- DONE: re-read code and put fixed sized elemenets where they are due to this thing is platform independent. i dont know if its wise to use the fast versions.

- DONE: check if you cna change write buffer to void* 

- WARNING: apparently fopen can only keep like 900 FILE* alive at once....fix this only if you have the time, honestly. but if you do have the time, fix it first.

- DONE(?): files open in w+ mode. if i want to append to a file, and it didnt exist, great, it works. if i want to append to a file and it is already open, great, it works. if i want to read from a file that's already open, fantastic, it works. if i want to replace a file that is not open, great, it works. but if u want to append to a file that is closed, you've a problem, youd have to open in r+ mode, so that u dont delete it. however if a write() later on is going to replace, then your only choice is to close and reopen in wb+ again OR close the file, remove it, and then reopen. you might need a structure or a flag for this...look into fctnl

- DONE(?): also make sure to use fseek and rewind and all that.

- since there's only gonan be one thread per bucket anyway, you should make it so that in the internal logic of the thread it handles file_id's instead of filenames, so that you might save yourself the trie query.

# 2020-10-15
- so, the idea i said before would work cuz queries would arrive as filenames anyway. a thing i COULD do is check is send both the finfo and filename to the client and recieve both, then when a file is deleted i mark something that says "finfo is not up to date", i mark it as "up to date" when i recieve a query that corresponds to that finfo with the correct filename, then i can perform operations with the finfo instead....until another delete arrives for that finfo....id also have to like, asynchronously notify the thread that is sending stuff to tell the client the updated finfo numbers every so often....

- the thread that is writing to all the queues should have a wait on being able to write to the queue and on there being space available for that queue....but definitely not on the thread itself.

# 2020-10-17
- you can make it all in C if you make szdcue take only uint16_t, and if u make a macro that allows you to declare a szdcue struct with a suffix and a size.

# 2020-10-18
- in poll_cons, the else from the `avail.items > 0` if shouldnt necessarily send an error if there are multiple poll_conns threads. rather youd have a global connections counter or something, and if THAT was exceeded, then you'd wait for errs

- poll_conns' semaphores would also prolly need a mutex if youre gonna have several of them running at once

# 2020-10-19
- you can make szdcue generic with a macro that takes a size, a type, and a suffix.you can save yourself the trouble or writing a bunch of functions too, since a pointer to a struct can be cast into a pointer to the first attribute. you can make a szdcue who's first attribute is a struct containing its head and tail and make functions that modify those. \\X

# 2020-10-20
- if youre only going to have one error sender (consumer) but multiple error producers then i think u only need mutex in the producers. but idk, so im gonna put it in both. think later.

- the first instinct of every failure in the server shoudl be to send an error to the client, so make sure you guard against the possibility that the client has already closed the connection.

- handle server disconnection from client in client

- handle cleanup of thread pool

# 2020-10-22
- add a thing to reset the open file in client \\X

- epoll should not react to sockets that are being addressed by a bucket handler \\X

# 2020-10-24
- you could make it so each chunk of data has an ok and an err at the beginning...but there's no guarantee that they'll get there at the beginning of a chunk...

# 2020-10-25
- the buck_handler posts to buck_spaces even tho it doesnt really need to. if there were several poll_conns tho, it would make sense in case the array was full at the time of popping, so that all threads might get a chance to put their request in. that being said, new requests that are just being parse will be met with a -2 the read_client at this point.
