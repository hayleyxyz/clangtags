#include <iostream>
#include <clang-c/Index.h>
#include <sstream>
#include <clang-c/CXString.h>
#include "json.hpp"

using json = nlohmann::json;

std::ostream& operator<<(std::ostream& stream, const CXString& str) {
    auto cstr = clang_getCString(str);
    if(cstr == nullptr) {
        stream << "<NULL>";
    }
    else {
        stream << cstr;
    }
    //clang_disposeString(str);
    return stream;
}

void to_json(json& j, const CXString& str) {
    auto cstr = clang_getCString(str);
    if(cstr != nullptr) {
        j = cstr;
    }
    else {
        j = nullptr;
    }
    //clang_disposeString(str);
}

void from_json(const json& j, CXString& str) {
    throw std::runtime_error("unimplemented");
}

class ManagedCXString {
public:
    explicit ManagedCXString(CXString str) : str(str) {}
    ~ManagedCXString() {
        clang_disposeString(str);
    }

    operator CXString() const { return str; }

    CXString str{nullptr};
};

CXChildVisitResult cursor_visitor(CXCursor cursor, CXCursor parent, CXClientData client_data) {
    auto location = clang_getCursorLocation(cursor);
    CXFile file;
    unsigned int line, column, offset;
    clang_getInstantiationLocation(location, &file, &line, &column, &offset);
    ManagedCXString fileName(clang_getFileName(file));

    auto extent = clang_getCursorExtent(cursor);
    auto start = clang_getRangeStart(extent);
    auto end = clang_getRangeEnd(extent);
    unsigned int startLine, startColumn, startOffset;
    unsigned int endLine, endColumn, endOffset;
    clang_getInstantiationLocation(start, nullptr, &startLine, &startColumn, &startOffset);
    clang_getInstantiationLocation(end, nullptr, &endLine, &endColumn, &endOffset);

    auto language = clang_getCursorLanguage(cursor);
    auto kind = clang_getCursorKind(cursor);
    ManagedCXString kindName(clang_getCursorKindSpelling(kind));
    auto type = clang_getCursorType(cursor);
    ManagedCXString typeName(clang_getTypeSpelling(type));
    ManagedCXString spelling(clang_getCursorSpelling(cursor));
    ManagedCXString displayName(clang_getCursorDisplayName(cursor));
    auto isDefinition = static_cast<bool>(clang_isCursorDefinition(cursor));
    auto definition = clang_getCursorDefinition(cursor);

    auto isStatic = static_cast<bool>(clang_CXXMethod_isStatic(cursor));
    auto isReference = static_cast<bool>(clang_isReference(cursor.kind));
    auto referenced = clang_getCursorReferenced(cursor);

    ManagedCXString usr(clang_getCursorUSR(cursor));

    json *pjson = static_cast<json*>(client_data);

    json j;
    j["location"] = json::object({ { "fileName", fileName }, { "line", line }, { "column", column }, { "offset", offset } });
    j["extent"] = json::object({
        { "start", {
            { "line", startLine },
            { "column", startColumn },
            { "offset", startOffset }
        }},
        { "end", {
           { "line", endLine },
           { "column", endColumn },
           { "offset", endOffset }
      }},
    });
    j["kind"] = kind;
    j["kind_name"] = kindName;
    j["type"] = type.kind;
    j["type_name"] = typeName;
    j["spelling"] = spelling;
    j["display"] = displayName;
    j["is_definition"] = isDefinition;

    if(!clang_Cursor_isNull(definition)) {
        ManagedCXString definitionUSR(clang_getCursorUSR(definition));
        j["definition"] = definitionUSR;
    }
    else {
        j["definition"] = nullptr;
    }

    j["is_static"] = isStatic;
    j["is_reference"] = isReference;
    j["usr"] = usr;

    if(!clang_Cursor_isNull(referenced)) {
        ManagedCXString referencedUSR(clang_getCursorUSR(referenced));
        j["referencedUSR"] = referencedUSR;
    }
    else {
        j["referencedUSR"] = nullptr;
    }

    switch(language) {
        case CXLanguage_C:
            j["language"] = "c";
            break;
        case CXLanguage_CPlusPlus:
            j["language"] = "cpp";
            break;
        default:
            j["language"] = nullptr;
            break;
    }

    pjson->push_back(j);
    //std::cout << j.dump(2) << std::endl;

    return CXChildVisit_Recurse;
}

int main(int argc, char *argv[]) {
    if(argc < 2) {
        throw std::runtime_error("argc < 2");
    }

    auto index = clang_createIndex(false, 0);

    const char *pargs[] = {
            "-I/Users/oscar/Projects/zircon/build-arm64/gen/global/include",
            "-I/Users/oscar/Projects/zircon/build-arm64/system/uapp/psutils.memgraph/include",
            "-I/Users/oscar/Projects/zircon/build-arm64/system/fidl/fuchsia-ldsvc/gen/include",
            "-I/Users/oscar/Projects/zircon/build-arm64/system/fidl/fuchsia-mem/gen/include",
            "-I/Users/oscar/Projects/zircon/build-arm64/system/fidl/fuchsia-io/gen/include",
            "-I/Users/oscar/Projects/zircon/build-arm64/system/fidl/fuchsia-logger/gen/include",
            "-I/Users/oscar/Projects/zircon/build-arm64/system/fidl/fuchsia-display/gen/include",
            "-I/Users/oscar/Projects/zircon/build-arm64/system/fidl/fuchsia-tracelink/gen/include",
            "-I/Users/oscar/Projects/zircon/build-arm64/system/fidl/fuchsia-process/gen/include",
            "-I/Users/oscar/Projects/zircon/build-arm64/system/fidl/fuchsia-net-stack/gen/include",
            "-I/Users/oscar/Projects/zircon/build-arm64/system/fidl/fuchsia-net/gen/include",
            "-I/Users/oscar/Projects/zircon/build-arm64/system/fidl/fuchsia-cobalt/gen/include",
            "-I/Users/oscar/Projects/zircon/build-arm64/system/fidl/fuchsia-crash/gen/include",
            "-I/Users/oscar/Projects/zircon/build-arm64/system/fidl/fuchsia-sysmem/gen/include",
            "-I/Users/oscar/Projects/zircon/build-arm64/system/fidl/fuchsia-camera/gen/include",
            "-I/Users/oscar/Projects/zircon/build-arm64/sysroot/include",
            "-I/Users/oscar/Projects/zircon/bootloader/include",
            "-I/Users/oscar/Projects/zircon/prebuilt/downloads/clang/include",
            "-I/Users/oscar/Projects/zircon/prebuilt/downloads/clang/lib/clang/7.0.0/include",
            "-I/Users/oscar/Projects/zircon/prebuilt/downloads/clang/lib/x86_64-fuchsia/include",
            "-I/Users/oscar/Projects/zircon/prebuilt/downloads/clang/lib/aarch64-fuchsia/include",
            "-I/Users/oscar/Projects/zircon/prebuilt/downloads/gcc/lib/gcc/x86_64-elf/8.1.1/include",
            "-I/Users/oscar/Projects/zircon/prebuilt/downloads/gcc/lib/gcc/x86_64-elf/8.1.1/plugin/include",
            "-I/Users/oscar/Projects/zircon/prebuilt/downloads/gcc/lib/gcc/x86_64-elf/8.1.1/install-tools/include",
            "-I/Users/oscar/Projects/zircon/prebuilt/downloads/gcc/lib/gcc/aarch64-elf/8.1.1/include",
            "-I/Users/oscar/Projects/zircon/prebuilt/downloads/gcc/lib/gcc/aarch64-elf/8.1.1/plugin/include",
            "-I/Users/oscar/Projects/zircon/prebuilt/downloads/gcc/lib/gcc/aarch64-elf/8.1.1/install-tools/include",
            "-I/Users/oscar/Projects/zircon/system/ulib/zx/include",
            "-I/Users/oscar/Projects/zircon/system/ulib/framebuffer/include",
            "-I/Users/oscar/Projects/zircon/system/ulib/chromeos-disk-setup/include",
            "-I/Users/oscar/Projects/zircon/system/ulib/fs-host/include",
            "-I/Users/oscar/Projects/zircon/system/ulib/loader-service/include",
            "-I/Users/oscar/Projects/zircon/system/ulib/crypto/include",
            "-I/Users/oscar/Projects/zircon/system/ulib/edid/include",
            "-I/Users/oscar/Projects/zircon/system/ulib/unittest/include",
            "-I/Users/oscar/Projects/zircon/system/ulib/test-utils/include",
            "-I/Users/oscar/Projects/zircon/system/ulib/zxcrypt/include",
            "-I/Users/oscar/Projects/zircon/system/ulib/fs-management/include",
            "-I/Users/oscar/Projects/zircon/system/ulib/trace-provider/include",
            "-I/Users/oscar/Projects/zircon/system/ulib/syslog/include",
            "-I/Users/oscar/Projects/zircon/system/ulib/async-testutils/include",
            "-I/Users/oscar/Projects/zircon/system/ulib/mini-process/include",
            "-I/Users/oscar/Projects/zircon/system/ulib/trace/include",
            "-I/Users/oscar/Projects/zircon/system/ulib/audio-driver-proto/include",
            "-I/Users/oscar/Projects/zircon/system/ulib/utf_conversion/include",
            "-I/Users/oscar/Projects/zircon/system/ulib/memfs/include",
            "-I/Users/oscar/Projects/zircon/system/ulib/virtio/include",
            "-I/Users/oscar/Projects/zircon/system/ulib/fit/include",
            "-I/Users/oscar/Projects/zircon/system/ulib/gpt/include",
            "-I/Users/oscar/Projects/zircon/system/ulib/hwreg/include",
            "-I/Users/oscar/Projects/zircon/system/ulib/inspector/include",
            "-I/Users/oscar/Projects/zircon/system/ulib/vmo-utils/include",
            "-I/Users/oscar/Projects/zircon/system/ulib/trace-engine/include",
            "-I/Users/oscar/Projects/zircon/system/ulib/bitmap/include",
            "-I/Users/oscar/Projects/zircon/system/ulib/driver/include",
            "-I/Users/oscar/Projects/zircon/system/ulib/fbl/include",
            "-I/Users/oscar/Projects/zircon/system/ulib/ldmsg/include",
            "-I/Users/oscar/Projects/zircon/system/ulib/blktest/include",
            "-I/Users/oscar/Projects/zircon/system/ulib/ddk/include",
            "-I/Users/oscar/Projects/zircon/system/ulib/digest/include",
            "-I/Users/oscar/Projects/zircon/system/ulib/bootdata/include",
            "-I/Users/oscar/Projects/zircon/system/ulib/zircon/include",
            "-I/Users/oscar/Projects/zircon/system/ulib/async/include",
            "-I/Users/oscar/Projects/zircon/system/ulib/smbios/include",
            "-I/Users/oscar/Projects/zircon/system/ulib/dispatcher-pool/include",
            "-I/Users/oscar/Projects/zircon/system/ulib/runtime/include",
            "-I/Users/oscar/Projects/zircon/system/ulib/elfload/include",
            "-I/Users/oscar/Projects/zircon/system/ulib/intel-hda/include",
            "-I/Users/oscar/Projects/zircon/system/ulib/fs-test-utils/include",
            "-I/Users/oscar/Projects/zircon/system/ulib/fdio/include",
            "-I/Users/oscar/Projects/zircon/system/ulib/tee-client-api/include",
            "-I/Users/oscar/Projects/zircon/system/ulib/mdns/include",
            "-I/Users/oscar/Projects/zircon/system/ulib/fzl/include",
            "-I/Users/oscar/Projects/zircon/system/ulib/perftest/include",
            "-I/Users/oscar/Projects/zircon/system/ulib/explicit-memory/include",
            "-I/Users/oscar/Projects/zircon/system/ulib/svc/include",
            "-I/Users/oscar/Projects/zircon/system/ulib/async-loop/include",
            "-I/Users/oscar/Projects/zircon/system/ulib/launchpad/include",
            "-I/Users/oscar/Projects/zircon/system/ulib/tftp/include",
            "-I/Users/oscar/Projects/zircon/system/ulib/block-client/include",
            "-I/Users/oscar/Projects/zircon/system/ulib/trace-reader/include",
            "-I/Users/oscar/Projects/zircon/system/ulib/kvstore/include",
            "-I/Users/oscar/Projects/zircon/system/ulib/driver-info/include",
            "-I/Users/oscar/Projects/zircon/system/ulib/rtc/include",
            "-I/Users/oscar/Projects/zircon/system/ulib/blobfs/include",
            "-I/Users/oscar/Projects/zircon/system/ulib/audio-proto-utils/include",
            "-I/Users/oscar/Projects/zircon/system/ulib/xdc-server-utils/include",
            "-I/Users/oscar/Projects/zircon/system/ulib/hid/include",
            "-I/Users/oscar/Projects/zircon/system/ulib/zxcpp/include",
            "-I/Users/oscar/Projects/zircon/system/ulib/inet6/include",
            "-I/Users/oscar/Projects/zircon/system/ulib/pretty/include",
            "-I/Users/oscar/Projects/zircon/system/ulib/fidl/include",
            "-I/Users/oscar/Projects/zircon/system/ulib/sync/include",
            "-I/Users/oscar/Projects/zircon/system/ulib/hid-parser/include",
            "-I/Users/oscar/Projects/zircon/system/ulib/port/include",
            "-I/Users/oscar/Projects/zircon/system/ulib/process-launcher/include",
            "-I/Users/oscar/Projects/zircon/system/ulib/fs/include",
            "-I/Users/oscar/Projects/zircon/system/ulib/runtests-utils/include",
            "-I/Users/oscar/Projects/zircon/system/ulib/gfx/include",
            "-I/Users/oscar/Projects/zircon/system/ulib/region-alloc/include",
            "-I/Users/oscar/Projects/zircon/system/ulib/zbi/include",
            "-I/Users/oscar/Projects/zircon/system/ulib/task-utils/include",
            "-I/Users/oscar/Projects/zircon/system/ulib/audio-utils/include",
            "-I/Users/oscar/Projects/zircon/system/ulib/ddktl/include",
            "-I/Users/oscar/Projects/zircon/system/ulib/fvm/include",
            "-I/Users/oscar/Projects/zircon/system/ulib/minfs/include",
            "-I/Users/oscar/Projects/zircon/system/core/crashanalyzer/include",
            "-I/Users/oscar/Projects/zircon/system/host/fidl/include",
            "-I/Users/oscar/Projects/zircon/system/host/fvm/include",
            "-I/Users/oscar/Projects/zircon/system/utest/fbl/include",
            "-I/Users/oscar/Projects/zircon/system/dev/lib/amlogic/include",
            "-I/Users/oscar/Projects/zircon/system/dev/lib/hi3660/include",
            "-I/Users/oscar/Projects/zircon/system/dev/lib/imx8m/include",
            "-I/Users/oscar/Projects/zircon/system/dev/gpio/pl061/include",
            "-I/Users/oscar/Projects/zircon/system/dev/clk/meson-lib/include",
            "-I/Users/oscar/Projects/zircon/third_party/ulib/musl/include",
            "-I/Users/oscar/Projects/zircon/third_party/ulib/musl/third_party/include",
            "-I/Users/oscar/Projects/zircon/third_party/ulib/ngunwind/include",
            "-I/Users/oscar/Projects/zircon/third_party/ulib/backtrace/include",
            "-I/Users/oscar/Projects/zircon/third_party/ulib/jemalloc/test/include",
            "-I/Users/oscar/Projects/zircon/third_party/ulib/jemalloc/include",
            "-I/Users/oscar/Projects/zircon/third_party/ulib/cryptolib/include",
            "-I/Users/oscar/Projects/zircon/third_party/ulib/linenoise/include",
            "-I/Users/oscar/Projects/zircon/third_party/ulib/chromiumos-platform-ec/include",
            "-I/Users/oscar/Projects/zircon/third_party/ulib/usb-dwc-regs/include",
            "-I/Users/oscar/Projects/zircon/third_party/ulib/qrcodegen/include",
            "-I/Users/oscar/Projects/zircon/third_party/ulib/lz4/include",
            "-I/Users/oscar/Projects/zircon/third_party/ulib/cksum/include",
            "-I/Users/oscar/Projects/zircon/third_party/ulib/uboringssl/include",
            "-I/Users/oscar/Projects/zircon/third_party/lib/acpica/source/include",
            "-I/Users/oscar/Projects/zircon/third_party/lib/jitterentropy/include",
            "-I/Users/oscar/Projects/zircon/build-x64/gen/global/include",
            "-I/Users/oscar/Projects/zircon/build-x64/system/uapp/psutils.memgraph/include",
            "-I/Users/oscar/Projects/zircon/build-x64/system/fidl/fuchsia-ldsvc/gen/include",
            "-I/Users/oscar/Projects/zircon/build-x64/system/fidl/fuchsia-mem/gen/include",
            "-I/Users/oscar/Projects/zircon/build-x64/system/fidl/fuchsia-io/gen/include",
            "-I/Users/oscar/Projects/zircon/build-x64/system/fidl/fuchsia-logger/gen/include",
            "-I/Users/oscar/Projects/zircon/build-x64/system/fidl/fuchsia-display/gen/include",
            "-I/Users/oscar/Projects/zircon/build-x64/system/fidl/fuchsia-tracelink/gen/include",
            "-I/Users/oscar/Projects/zircon/build-x64/system/fidl/fuchsia-process/gen/include",
            "-I/Users/oscar/Projects/zircon/build-x64/system/fidl/fuchsia-net-stack/gen/include",
            "-I/Users/oscar/Projects/zircon/build-x64/system/fidl/fuchsia-net/gen/include",
            "-I/Users/oscar/Projects/zircon/build-x64/system/fidl/fuchsia-cobalt/gen/include",
            "-I/Users/oscar/Projects/zircon/build-x64/system/fidl/fuchsia-crash/gen/include",
            "-I/Users/oscar/Projects/zircon/build-x64/system/fidl/fuchsia-sysmem/gen/include",
            "-I/Users/oscar/Projects/zircon/build-x64/system/fidl/fuchsia-camera/gen/include",
            "-I/Users/oscar/Projects/zircon/build-x64/sysroot/include",
            "-I/Users/oscar/Projects/zircon/kernel/include",
            "-I/Users/oscar/Projects/zircon/kernel/platform/pc/include",
            "-I/Users/oscar/Projects/zircon/kernel/vm/include",
            "-I/Users/oscar/Projects/zircon/kernel/object/include",
            "-I/Users/oscar/Projects/zircon/kernel/arch/x86/include",
            "-I/Users/oscar/Projects/zircon/kernel/arch/x86/page_tables/include",
            "-I/Users/oscar/Projects/zircon/kernel/arch/arm64/include",
            "-I/Users/oscar/Projects/zircon/kernel/lib/libc/include",
            "-I/Users/oscar/Projects/zircon/kernel/lib/crypto/include",
            "-I/Users/oscar/Projects/zircon/kernel/lib/pci/include",
            "-I/Users/oscar/Projects/zircon/kernel/lib/unittest/include",
            "-I/Users/oscar/Projects/zircon/kernel/lib/vdso/include",
            "-I/Users/oscar/Projects/zircon/kernel/lib/fbl/include",
            "-I/Users/oscar/Projects/zircon/kernel/lib/pow2_range_allocator/include",
            "-I/Users/oscar/Projects/zircon/kernel/lib/io/include",
            "-I/Users/oscar/Projects/zircon/kernel/lib/debuglog/include",
            "-I/Users/oscar/Projects/zircon/kernel/lib/oom/include",
            "-I/Users/oscar/Projects/zircon/kernel/lib/cbuf/include",
            "-I/Users/oscar/Projects/zircon/kernel/lib/watchdog/include",
            "-I/Users/oscar/Projects/zircon/kernel/lib/version/include",
            "-I/Users/oscar/Projects/zircon/kernel/lib/heap/include",
            "-I/Users/oscar/Projects/zircon/kernel/lib/heap/cmpctmalloc/include",
            "-I/Users/oscar/Projects/zircon/kernel/lib/memory_limit/include",
            "-I/Users/oscar/Projects/zircon/kernel/lib/user_copy/include",
            "-I/Users/oscar/Projects/zircon/kernel/lib/code_patching/include",
            "-I/Users/oscar/Projects/zircon/kernel/lib/hypervisor/include",
            "-I/Users/oscar/Projects/zircon/kernel/lib/fixed_point/include",
            "-I/Users/oscar/Projects/zircon/kernel/dev/udisplay/include",
            "-I/Users/oscar/Projects/zircon/kernel/dev/pdev/include",
            "-I/Users/oscar/Projects/zircon/kernel/dev/timer/arm_generic/include",
            "-I/Users/oscar/Projects/zircon/kernel/dev/interrupt/include",
            "-I/Users/oscar/Projects/zircon/kernel/dev/interrupt/arm_gic/v2/include",
            "-I/Users/oscar/Projects/zircon/kernel/dev/interrupt/arm_gic/common/include",
            "-I/Users/oscar/Projects/zircon/kernel/dev/interrupt/arm_gic/v3/include",
            "-I/Users/oscar/Projects/zircon/kernel/dev/iommu/intel/include",
            "-I/Users/oscar/Projects/zircon/kernel/dev/iommu/dummy/include",
            "-I/Users/oscar/Projects/zircon/kernel/dev/psci/include",
            "-I/Users/oscar/Projects/zircon/kernel/dev/pcie/include",
            "-I/Users/oscar/Projects/zircon/kernel/dev/hw_rng/include",

            "-include",
            "/Users/oscar/Projects/zircon/build-x64/config-kernel.h"
    };

    CXTranslationUnit unit;
    CXErrorCode err = clang_parseTranslationUnit2(
            index,
            //argv[1],
            //pargs, 186 + 2,
            nullptr,
            reinterpret_cast<const char *const *>(argv + 1), argc - 1,
            nullptr, 0,
            CXTranslationUnit_DetailedPreprocessingRecord | CXTranslationUnit_KeepGoing,
            &unit);

    if(err != CXError_Success) {
        std::ostringstream out;
        out << "failed to create parse translation unit. err: ";
        out << err;
        throw std::runtime_error(out.str());
    }

    json j;

    CXCursor cursor = clang_getTranslationUnitCursor(unit);
    clang_visitChildren(
            cursor,
            cursor_visitor,
            &j);

    clang_disposeTranslationUnit(unit);
    clang_disposeIndex(index);

    std::cout << j.dump(2) << std::endl;

    return 0;
}