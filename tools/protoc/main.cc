// Minimal protoc: the C++ generator only, built from the static libprotoc/libprotobuf
// shipped in protobuf.zip. See tools/build-protoc.sh for why this exists.
#include <google/protobuf/compiler/command_line_interface.h>
#include <google/protobuf/compiler/cpp/generator.h>

int main(int argc, char* argv[]) {
    google::protobuf::compiler::CommandLineInterface cli;
    cli.AllowPlugins("protoc-");
    google::protobuf::compiler::cpp::CppGenerator cpp_generator;
    cli.RegisterGenerator("--cpp_out", "--cpp_opt", &cpp_generator, "Generate C++ header and source.");
    return cli.Run(argc, argv);
}
