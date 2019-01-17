#include "Load.hpp"

#include <array>
#include <list>
#include <cassert>

namespace {
	std::array< std::list< std::function< void() > >, LoadTagCount > &get_load_lists() {
		static std::array< std::list< std::function< void() > >, LoadTagCount > load_lists;
		return load_lists;
	}
}

void add_load_function(LoadTag tag, std::function< void() > const &fn) {
	auto &load_lists = get_load_lists();
	assert(tag < load_lists.size());
	load_lists[tag].emplace_back(fn);
}

void call_load_functions() {
	auto &load_lists = get_load_lists();
	for (auto &fn_list : load_lists) {
		while (!fn_list.empty()) {
			(*fn_list.begin())(); //call first function in the list
			fn_list.pop_front(); //remove from list
		}
	}
}
