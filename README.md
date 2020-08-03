# pspawn
C++17 header-only library to spawn processes

## Snippet

```cpp
#include <iostream>
#include <pspawn/pspawn.hpp>

int main(int argc, char* argv[]) {

  if(argc != 2) {
    std::cout << "Specify program to spawn" <<std::endl;
    return -1;
  }
    
  using namespace pspawn;
  auto p = process::spawn(argv[1]);
  // auto p = process::spawn(argv[1], "", priority::low);

  if (!p) {
    std::cout << process::last_error().message() << std::endl;
    return -1;
  }

  if (!p.done())
    std::cout << "Process is running" << std::endl;

  p.wait();
  // p.wait_for(std::chrono::milliseconds{3000});

  auto const maybe_code = p.exit_code();
  if(!!maybe_code)
    std::cout << "Exit code: " << *maybe_code << std::endl;
    
  return 0;
}
```
