// Author: melty <11942219+meltybk@users.noreply.github.com>

#pragma once
#ifndef ANALYZER_H_
#define ANALYZER_H_

#include <string>
#include <vector>

namespace clang {

class NamespaceDecl;
class CXXRecordDecl;
class FunctionDecl;
class FieldDecl;

}  // namespace clang

struct Namespace {
  std::string name_;
};

struct Parameter {
  std::string type_;
  std::string name_;
};

struct Function {
  std::string name_;
  std::string return_type_;
  bool virtual_{false};
  std::vector<Parameter> parameters_;
};

struct Field {
  std::string type_;
  std::string name_;
};

struct CXXRecord {
  enum class Kind : uint8_t {
    kStruct,
    kInterface,
    kUnion,
    kClass,
    kEnum,
  };

  std::string name_;
  std::string qualified_name_;
  std::vector<Function> functions_;
  std::vector<Field> fields_;
  Kind kind_;
  CXXRecord *base_{nullptr};
};

/// Analyze C++ source code by Clang AST.
class Analyzer final {
 public:
  /// Constructor.
  Analyzer();

  /// Destructor.
  ~Analyzer();

  /// Disallow the copy and assign.
  Analyzer(const Analyzer &) = delete;
  Analyzer &operator=(const Analyzer &) = delete;

  /// Run AST C++ code analyzer.
  /// Specifi compile target cplusplus source file by compile_file.
  /// Will analyze classes that derived from this specified by base_class.
  /// If base_class is empty, analyze all classes.
  bool Analyze(const char *compile_file,
               const std::vector<const char *> &include_paths,
               const char *base_class);

  bool GenerateJSON(const char *output_path);

  void AddCXXRecord(clang::CXXRecordDecl *record_decl);
  void AddFunction(clang::CXXRecordDecl *record_decl,
                   clang::FunctionDecl *function_decl);
  void AddField(clang::CXXRecordDecl *record_decl,
                clang::FieldDecl *field_decl);

  inline const std::string &base_class() const { return base_class_; }

 private:
  /// Clang compile options.
   std::vector<std::string> options_;

  std::vector<CXXRecord> records_;

  std::string base_class_;
};

#endif  // ANALYZER_H_
