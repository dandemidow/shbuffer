import qbs
import "CxxStaticLibrary.qbs" as CxxStaticLibrary
import "CxxApplication.qbs" as CxxApplication

Project {
    CxxStaticLibrary {
        name: "sharedmem"
        files: [
            "src/shalloc.c",
            "include/sharedmem/shalloc.h",
            "src/buffer.c",
            "include/sharedmem/buffer.h"
        ]

        Group {
            fileTagsFilter: product.type
            qbs.install: true
            qbs.installDir: "/usr/lib"
        }
    }

    CxxApplication {
        name: "test"
        cpp.dynamicLibraries: [ "sharedmem", "rt", "pthread" ]
        files: "src/main.c"
    }

    CxxApplication {
        name: "reader"
        cpp.dynamicLibraries: [ "sharedmem", "rt", "pthread" ]
        files: "src/reader.c"
    }
}
