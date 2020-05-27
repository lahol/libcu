# libcu #
Small utility library for commonly used structures.

Although I am a huge fan of GLib, for some projects this is just to much. Furthermore,
I wanted to keep dependencies at a minimum. Hence this small standalone library.

## Modules ##
Currently, there are the following modules, in varying degrees of completeness. Most of
the time, only those aspects were implemented, that I required in another project (part-square-ems).

* **AVL Tree**

  A self-balancing binary tree that implements insertion, deletion, find in O(log(n)) and
  allows inorder traversal.

* **Fixed Stack**

  A stack with a given maximal number of elements of the same size.

* **Heap**

  A simple heap for unmanaged pointers.

* **List**

  A doubly-linked list.

* **Memory management**

  Wrappers for common (re)alloc/free and aligned memory. Also management of fixed size blocks
  in larger chunks. Each group access (alloc/free) can be done in O(1), accessing the groups
  in O(1) for alloc and O(log(n)) for free. Requires AVL tree and heap.

* **Mixed heap list**

  Manage data in a heap as well as a sorted list in order to manage neighbor relationships.

* **Queue**

  A simple queue in various flavors.
  * *Fixed Size Queue*: Manage objects with a fixed element size.
  * *Locked Queue*: Lock queue before access, MT-save.
  * *Queue*: Nothing special. Simple double-ended queue.

* **Stack**

  Simple stack implementation using a list.

* **Timer**

  Run callback functions perodically.

* **Arrays/Blobs**

  Pack data into a single type, not losing information of length and content types.
  For passing information around.

## License ##
This library is released under the terms of the MIT license. See LICENSE.
