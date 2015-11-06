import qbs
import "CxxStaticLibrary.qbs" as CxxStaticLibrary
import "CxxApplication.qbs" as CxxApplication

Project {
    CxxStaticLibrary {
        name: "sharedmem"
        files: [
            "src/shalloc.c",
            "src/buffer.c"
        ]

        Group {
            qbs.install: true
            qbs.installDir: "/usr/include/sharedmem"
            files: [
                "include/sharedmem/shalloc.h",
                "include/sharedmem/buffer.h"
            ]
        }
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
