## [](#header-1)Mooltipass Team Ground Rules  
  
### [](#header-3) Implement the functionalities decided by the group  
We all have differents way to think of how a particular feature should be made. As a consequence, the team relies on collective wisdom to pick the way we implement each functionality. This will be done via Google group discussions, and may be pushed to the general Mooltipass mailing list if a strong difference of opinions happen.  
Obviously this doesn't apply to trivial things, only to things that impact the global firmware architecture.

### [](#header-3) Use GitHub for code versioning and source control
We keep track of all changes that were made to the mooltipass source code and individually approve each change that will be applied to it.

### [](#header-3) Work on a dedicated file or folder for a given feature
We don't want several contributors changing at the same time a particular file, which would result in merge conflicts.

### [](#header-3) Document your code in doxygen format
We will not approve non documented code on github. Here is a simple example of correct documenting:

```c
/** Current detection state */
volatile uint8_t button_return;

/*!	\fn   security_code_validation(uint16_t code)
*	\brief Check security code
*	\param code	The code
*	\return success_status (see enum)
*/
RET_TYPE security_code_validation(uint16_t code)
```

### [](#header-3)C code format
- 4 spaces, no tabs
- snake case naming for functions and global variables
- defines in uppercase
- function names and global variables should be prepended by the file name it is in
- 80 chars limit is *not* needed (we're not in 1980 anymore!)
- a given function should not be more than 100 lines long (exceptions may happen)
- **comment your code**
- no magic numbers
- no mallocs
