/*
Copyright 2014 Google Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#ifndef DASHTOHLS_COMPATIBILITY_H_
#define DASHTOHLS_COMPATIBILITY_H_

#if C98
#define nullptr 0
#endif  // C98

#ifdef SHARED_PTR_INCLUDE
#include SHARED_PTR_INCLUDE
#endif  // SHARED_PTR_INCLUDE

#if C98
#ifdef SHARED_PTR_DEFINE
SHARED_PTR_DEFINE
#else  // SHARED_PTR_DEFINE
#error Define SHARED_PTR_DEFINE
#error Examples:
#error   -DSHARED_PTR_INCLUDE='\#include <boost::shared_ptr>;'
#error   -DSHARED_PTR_DEFINE='using boost::shared_ptr;'
#endif  // SHARED_PTR_DEFINE
#else  // C98
#include <memory>
using std::shared_ptr;
#endif  // C98

#endif  // DASHTOHLS_COMPATIBILITY_H_
