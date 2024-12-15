#include "helper.h"

class HelperImpl : public Helper {
public:
  uint32_t id() override { return ++_id ? _id : 1; }

private:
  uint32_t _id = 0;
};

std::shared_ptr<Helper> Helper::create() {
  return std::make_shared<HelperImpl>();
}
