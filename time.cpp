#include <ctime>
#include <string>

std::string fomat_time(std::time_t time) {
  if (time < 60) {
    return std::to_string(time) + "s";
  }

  if (time < 3600) {
    return std::to_string(time / 60) + "m";
  }

  return std::to_string(time / 3600) + "h" + " " + std::to_string(time % 3600 / 60) + "m";
}
