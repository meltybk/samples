// Author: melty <11942219+meltybk@users.noreply.github.com>

#include "analyzer.h"

// Disable warnings for clang.
#pragma warning(push)
#pragma warning(disable : 4100)
#pragma warning(disable : 4146)
#pragma warning(disable : 4244)
#pragma warning(disable : 4245)
#pragma warning(disable : 4267)
#pragma warning(disable : 4291)
#pragma warning(disable : 4324)
#pragma warning(disable : 4389)
#pragma warning(disable : 4456)
#pragma warning(disable : 4458)
#pragma warning(disable : 4459)
#pragma warning(disable : 4624)
#pragma warning(disable : 4702)
#pragma warning(disable : 4819)
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#pragma warning(pop)

#include "nlohmann/json.hpp"

#include <fstream>

class ClangASTFrontendAction;
class ClangASTConsumer;
class ClangASTVisitor;

template <typename T>
std::unique_ptr<clang::tooling::FrontendActionFactory> newFrontendActionFactory(
    Analyzer *analyzer);

Analyzer::Analyzer() {
}

Analyzer::~Analyzer() {
}

bool Analyzer::Analyze(const char *compile_file,
                       const std::vector<const char *> &include_paths,
                       const char *base_class) {
  assert(compile_file);

  if (base_class) {
    base_class_ = base_class;
  }

  options_.emplace_back("--");
  options_.emplace_back("clang++");
  for (const char *path : include_paths) {
    options_.emplace_back(path);
  }

  // Generate command line options.
  assert(compile_file && strlen(compile_file) > 0);
  int argc = 2;
  argc += static_cast<int>(options_.size());

  std::unique_ptr<const char *> argv(new const char *[argc]);
  argv.get()[0] = "";
  argv.get()[1] = compile_file;
  int index = 2;
  for (std::string &option : options_) {
    assert(index < argc);
    argv.get()[index] = option.c_str();
    ++index;
  }

  llvm::cl::OptionCategory option("");
  clang::tooling::CommonOptionsParser op(argc, argv.get(), option);
  clang::tooling::ClangTool tool(op.getCompilations(), op.getSourcePathList());

  // Run clang tooling
  tool.run(newFrontendActionFactory<ClangASTFrontendAction>(this).get());

  return true;
}

bool Analyzer::GenerateJSON(const char* output_path) {
  nlohmann::json json;

  json["class"] = nlohmann::json::array();

  for (CXXRecord &record : records_) {
    // Nerver dump base class.
    if (record.qualified_name_ == base_class_) {
      continue;
    }

    nlohmann::json record_json;
    record_json["name"] = record.name_;
    record_json["qualified_name"] = record.qualified_name_;

    record_json["field"] = nlohmann::json::array();
    for (const Field &field : record.fields_) {
      nlohmann::json filed_json;
      filed_json["name"] = field.name_;
      filed_json["type"] = field.type_;
      record_json["field"].push_back(filed_json);
    }

    record_json["function"] = nlohmann::json::array();
    for (const Function &function : record.functions_) {
      nlohmann::json function_json;
      function_json["name"] = function.name_;
      record_json["function"].push_back(function_json);
    }

    json["class"].push_back(record_json);
  }

  std::ofstream steam(output_path);

  std::string json_dump = json.dump(2);
  steam << json_dump;

  return true;
}

void Analyzer::AddCXXRecord(clang::CXXRecordDecl *record_decl) {
  records_.emplace_back();
  CXXRecord &record = records_.back();
  record.name_.assign(record_decl->getNameAsString());
  record.qualified_name_.assign(record_decl->getQualifiedNameAsString());

  // Determinate type
  switch (record_decl->getTagKind()) {
    case clang::TagTypeKind::TTK_Struct: {
      record.kind_ = CXXRecord::Kind::kStruct;
      break;
    }
    case clang::TagTypeKind::TTK_Interface: {
      record.kind_ = CXXRecord::Kind::kInterface;
      break;
    }
    case clang::TagTypeKind::TTK_Union: {
      record.kind_ = CXXRecord::Kind::kUnion;
      break;
    }
    case clang::TagTypeKind::TTK_Class: {
      record.kind_ = CXXRecord::Kind::kClass;
      break;
    }
    case clang::TagTypeKind::TTK_Enum: {
      record.kind_ = CXXRecord::Kind::kEnum;
      break;
    }
  }

  // Find base class decl.
  clang::CXXRecordDecl *base_decl = nullptr;
  clang::CXXBaseSpecifier* base_specifier = record_decl->bases_begin();
  if (base_specifier) {
    const clang::RecordType *rtype =
        base_specifier->getType()->getAs<clang::RecordType>();
    if (rtype) {
      base_decl = clang::cast_or_null<clang::CXXRecordDecl>(
          rtype->getDecl()->getDefinition());
      const std::string base_name = base_decl->getQualifiedNameAsString();

      for (CXXRecord &r : records_) {
        if (r.qualified_name_ == base_name) {
          record.base_ = &r;
          break;
        }
      }
    }
  }
}

void Analyzer::AddFunction(clang::CXXRecordDecl *record_decl,
                           clang::FunctionDecl *function_decl) {
  std::string record_name = record_decl->getNameAsString();
  for (CXXRecord &record : records_) {
    if (record.name_ != record_name) {
      continue;
    }

    std::string func_name = function_decl->getNameAsString();

    bool exist = false;
    for (Function &func : record.functions_) {
      if (func.name_ == func_name) {
        exist = true;
        break;
      }
    }

    if (!exist) {
      record.functions_.emplace_back();
      Function &func = record.functions_.back();
      func.name_ = function_decl->getNameAsString();
      func.return_type_ = function_decl->getReturnType().getAsString();
      func.virtual_ = function_decl->isVirtualAsWritten();

      clang::ArrayRef<clang::ParmVarDecl *> params =
          function_decl->parameters();
      for (clang::ParmVarDecl *parm_var_decl : params) {
        clang::QualType qual = parm_var_decl->getType();

        func.parameters_.emplace_back();
        Parameter &parameter = func.parameters_.back();
        parameter.type_ = qual.getAsString();
        parameter.name_ = parm_var_decl->getName().str();
      }
    }

    break;
  }
}

void Analyzer::AddField(clang::CXXRecordDecl *record_decl, clang::FieldDecl *field_decl) {
  std::string record_name = record_decl->getQualifiedNameAsString();
  for (CXXRecord &record : records_) {
    if (record.qualified_name_ != record_name) {
      continue;
    }

    std::string filed_name = field_decl->getNameAsString();
    record.fields_.emplace_back();
    Field &field = record.fields_.back();
    field.name_.assign(filed_name);

    clang::QualType qual = field_decl->getType();
    field.type_.assign(qual.getAsString());

    break;
  }
}

template <typename T>
std::unique_ptr<clang::tooling::FrontendActionFactory> newFrontendActionFactory(
    Analyzer *analyzer) {
  class ClangASTFrontendActionFactory
      : public clang::tooling::FrontendActionFactory {
   public:
    ClangASTFrontendActionFactory(Analyzer *analyzer) : analyzer_(analyzer) {}

    std::unique_ptr<clang::FrontendAction> create() override {
      return std::unique_ptr<clang::FrontendAction>(
          new ClangASTFrontendAction(analyzer_));
    }

   private:
    Analyzer *analyzer_{nullptr};
  };

  return std::unique_ptr<clang::tooling::FrontendActionFactory>(
      new ClangASTFrontendActionFactory(analyzer));
}

/// Clang AST Visitor implement class.
class ClangASTVisitor : public clang::RecursiveASTVisitor<ClangASTVisitor> {
 public:
  explicit ClangASTVisitor(clang::ASTContext &context, Analyzer *analyzer)
      : context_(context), analyzer_(analyzer) {}
  virtual ~ClangASTVisitor() {}

  bool VisitNamespaceDecl(clang::NamespaceDecl * /*decl*/) {
    //std::string name = decl->getName();
    //name.c_str();

    return true;
  }

  bool VisitCXXRecordDecl(clang::CXXRecordDecl *decl) {
    if (IsSpecifiedBaseClass(decl)) {
      object_decl_ = decl;
    } else {
      if (object_decl_ && decl->isThisDeclarationADefinition() &&
                              decl->isDerivedFrom(object_decl_)) {
        analyzer_->AddCXXRecord(decl);
      }
    }

    return true;
  }

  bool VisitFunctionDecl(clang::FunctionDecl *decl) {
    clang::DeclContext *parent = decl->getParent();
    if (parent) {
      clang::Decl::Kind kind = parent->getDeclKind();
      if (kind == clang::Decl::Kind::CXXRecord) {
        clang::CXXRecordDecl *cxx_record =
            static_cast<clang::CXXRecordDecl *>(parent);
        analyzer_->AddFunction(cxx_record, decl);
      }
    }

    return true;
  }

  bool VisitFieldDecl(clang::FieldDecl *decl) {
    clang::DeclContext *parent = decl->getParent();
    if (parent) {
      clang::Decl::Kind kind = parent->getDeclKind();
      if (kind == clang::Decl::Kind::CXXRecord) {
        clang::CXXRecordDecl *cxx_record =
            static_cast<clang::CXXRecordDecl *>(parent);
        analyzer_->AddField(cxx_record, decl);
      }
    }

    return true;
  }

 private:
  /// Return true when CXXRecordDecl is specified base class,
  /// Otherwise return false.
  bool IsSpecifiedBaseClass(clang::CXXRecordDecl *decl) {
    std::string name = decl->getQualifiedNameAsString();
    if (analyzer_->base_class() == decl->getQualifiedNameAsString()) {
      return true;
    }
    return false;
  }

  /// Clang AST context
  clang::ASTContext &context_;

  /// Entity class decl
  clang::CXXRecordDecl *object_decl_{nullptr};

  Analyzer *analyzer_{nullptr};
};

class ClangASTConsumer : public clang::ASTConsumer {
 public:
  explicit ClangASTConsumer(clang::CompilerInstance &ci, Analyzer *analyzer)
      : visitor_(ci.getASTContext(), analyzer) {}
  virtual ~ClangASTConsumer() {}

  virtual void HandleTranslationUnit(clang::ASTContext &context) override {
    visitor_.TraverseDecl(context.getTranslationUnitDecl());
  }

 private:
  ClangASTVisitor visitor_;
};

class ClangASTFrontendAction : public clang::ASTFrontendAction {
 public:
  ClangASTFrontendAction(Analyzer *analyzer) : analyzer_(analyzer) {}
   virtual ~ClangASTFrontendAction() {}

  virtual std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
      clang::CompilerInstance &ci, clang::StringRef) override {
    return std::unique_ptr<clang::ASTConsumer>(
        new ClangASTConsumer(ci, analyzer_));
  }

 private:
  Analyzer *analyzer_{nullptr};
};
