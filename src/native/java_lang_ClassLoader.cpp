/*
 * java_lang_ClassLoader.cpp
 *
 *  Created on: 2017年12月3日
 *      Author: zhengxiaolin
 */

#include "native/java_lang_ClassLoader.hpp"
#include <vector>
#include <algorithm>
#include <cassert>
#include "wind_jvm.hpp"
#include "native/native.hpp"
#include "native/java_lang_String.hpp"
#include "system_directory.hpp"
#include "classloader.hpp"

static unordered_map<wstring, void*> methods = {
    {L"findLoadedClass0:(" STR ")" CLS,				(void *)&JVM_FindLoadedClass},
    {L"findBootstrapClass:(" STR ")" CLS,				(void *)&JVM_FindBootStrapClass},
    {L"defineClass1:(" STR "[BIIL" PD STR ")" CLS,	(void *)&JVM_DefineClass1},
};

void JVM_FindLoadedClass(list<Oop *> & _stack){
	// find if the class has been loaded in the system_map:
	InstanceOop *_this = (InstanceOop *)_stack.front();	_stack.pop_front();
	InstanceOop *str = (InstanceOop *)_stack.front();	_stack.pop_front();

	wstring klass_name = java_lang_string::stringOop_to_wstring(str);		// 注意：这里传入的 str 是 Binary name。因此可以在 map 中直接查找～

	LockGuard lg(system_classmap_lock);

	auto iter = system_classmap.find(klass_name);
	if (iter == system_classmap.end()) {
		auto klass = MyClassLoader::get_loader().find_in_classmap(klass_name);		// fix: find in MyClassLoader, too.
		if (klass == nullptr) {
			_stack.push_back(nullptr);
		} else {
			_stack.push_back(klass->get_mirror());
		}
	} else {
		_stack.push_back(iter->second->get_mirror());
	}

#ifdef DEBUG
	if (_stack.back() == nullptr) {
		std::wcout << "(DEBUG) didn't find [" << klass_name << "] in system_map. return null." << std::endl;
	} else {
		std::wcout << "(DEBUG) finded [" << klass_name << "] in system_map. return its mirror." << std::endl;
	}
#endif

}

void JVM_FindBootStrapClass(list<Oop *> & _stack){
	// find if the class has been loaded in the system_map:
	InstanceOop *_this = (InstanceOop *)_stack.front();	_stack.pop_front();
	InstanceOop *str = (InstanceOop *)_stack.front();	_stack.pop_front();

	wstring klass_name = java_lang_string::stringOop_to_wstring(str);		// 注意：这里传入的 str 是 Binary name。因此可以在 map 中直接查找～

	LockGuard lg(system_classmap_lock);

	auto klass = BootStrapClassLoader::get_bootstrap().loadClass(klass_name);
	if (klass == nullptr) {
		_stack.push_back(nullptr);
	} else {
		_stack.push_back(klass->get_mirror());
	}

#ifdef DEBUG
	if (_stack.back() == nullptr) {
		std::wcout << "(DEBUG) didn't find [" << klass_name << "] in system_map. return null." << std::endl;
	} else {
		std::wcout << "(DEBUG) finded [" << klass_name << "] in system_map. return its mirror." << std::endl;
	}
#endif
}

void JVM_DefineClass1(list<Oop *> & _stack){
	InstanceOop *_this = (InstanceOop *)_stack.front();	_stack.pop_front();
	InstanceOop *name = (InstanceOop *)_stack.front();	_stack.pop_front();
	TypeArrayOop *bytes = (TypeArrayOop *)_stack.front();	_stack.pop_front();
	int offset = ((IntOop *)_stack.front())->value;	_stack.pop_front();
	int len = ((IntOop *)_stack.front())->value;	_stack.pop_front();
	InstanceOop *protection_domain = (InstanceOop *)_stack.front();	_stack.pop_front();		// 我忽略掉这东西了。安全机制就算了。
	InstanceOop *source_str = (InstanceOop *)_stack.front();	_stack.pop_front();				// 这东西也算了。不过如果不是 nullptr，可以打印出来玩玩。

	wstring klass_name = java_lang_string::stringOop_to_wstring(name);

	auto klass = MyClassLoader::get_loader().loadClass(klass_name);
	assert(klass != nullptr);
	_stack.push_back(klass->get_mirror());

#ifdef DEBUG
	std::wcout << "(DEBUG) DefineClass1(): [" << klass_name << "]." << std::endl;
#endif
}


// 返回 fnPtr.
void *java_lang_classLoader_search_method(const wstring & signature)
{
	auto iter = methods.find(signature);
	if (iter != methods.end()) {
		return (*iter).second;
	}
	return nullptr;
}

