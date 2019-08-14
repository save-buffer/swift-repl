#ifndef LIBRARY_LOADING_H
#define LIBRARY_LOADING_H

#include <string>

void AddLibrarySearchPath(std::string path);
// Returns true on success, false on failure
bool SearchForAndLoadLibraryPermanently(std::string name);

#endif
