//
// Created by zach_contactci on 2/2/2022.
//

#pragma once

#define DllExport   __declspec( dllexport )
#define DllImport   __declspec( dllimport )

namespace contactci {
    namespace core {
        class Exports {
        public:
            DllExport static void say_hello();
        };
    }
}
