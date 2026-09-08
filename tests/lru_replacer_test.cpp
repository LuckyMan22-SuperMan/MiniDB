#include "minidb/lru_replacer.h"

#include <cassert>

int main() {
    minidb::LRUReplacer replacer(2);

    replacer.unpin(0);
    replacer.unpin(1);
    assert(replacer.size() == 2);

    replacer.pin(0);
    assert(replacer.size() == 1);
    replacer.unpin(0);

    const auto first_victim = replacer.victim();
    assert(first_victim.has_value());
    assert(*first_victim == 1);

    const auto second_victim = replacer.victim();
    assert(second_victim.has_value());
    assert(*second_victim == 0);
    assert(!replacer.victim().has_value());
    return 0;
}
