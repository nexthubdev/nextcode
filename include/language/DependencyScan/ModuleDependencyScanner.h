//===--- ModuleDependencyScanner.h - Import Swift modules --------*- C++
//-*-===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2020 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//

#include "language/AST/ASTContext.h"
#include "language/AST/Identifier.h"
#include "language/AST/ModuleDependencies.h"
#include "language/Frontend/ModuleInterfaceLoader.h"
#include "language/Serialization/SerializedModuleLoader.h"
#include "llvm/CAS/CASReference.h"
#include "llvm/Support/ThreadPool.h"

namespace language {
class DependencyTracker;
}

namespace language {

/// A dependency scanning worker which performs filesystem lookup
/// of a named module dependency.
class ModuleDependencyScanningWorker {
public:
  ModuleDependencyScanningWorker(
      SwiftDependencyScanningService &globalScanningService,
      const CompilerInvocation &ScanCompilerInvocation,
      const SILOptions &SILOptions, ASTContext &ScanASTContext,
      DependencyTracker &DependencyTracker, DiagnosticEngine &diags);

private:
  /// Retrieve the module dependencies for the Clang module with the given name.
  ModuleDependencyVector scanFilesystemForClangModuleDependency(
      Identifier moduleName, StringRef moduleOutputPath,
      StringRef sdkModuleOutputPath,
      const llvm::DenseSet<clang::tooling::dependencies::ModuleID>
          &alreadySeenModules,
      llvm::PrefixMapper *prefixMapper);

  /// Retrieve the module dependencies for the Swift module with the given name.
  ModuleDependencyVector scanFilesystemForSwiftModuleDependency(
      Identifier moduleName, StringRef moduleOutputPath,
      StringRef sdkModuleOutputPath, llvm::PrefixMapper *prefixMapper,
      bool isTestableImport = false);

  /// Store cache entry for include tree.
  llvm::Error
  createCacheKeyForEmbeddedHeader(std::string embeddedHeaderIncludeTree,
                                  std::string chainedHeaderIncludeTree);

  // Worker-specific instance of CompilerInvocation
  std::unique_ptr<CompilerInvocation> workerCompilerInvocation;
  // Worker-specific instance of ASTContext
  std::unique_ptr<ASTContext> workerASTContext;
  // An AST delegate for interface scanning.
  std::unique_ptr<InterfaceSubContextDelegateImpl> scanningASTDelegate;
  // The Clang scanner tool used by this worker.
  clang::tooling::dependencies::DependencyScanningTool clangScanningTool;
  // Swift and Clang module loaders acting as scanners.
  std::unique_ptr<ModuleInterfaceLoader> swiftScannerModuleLoader;
  std::unique_ptr<ClangImporter> clangScannerModuleLoader;
  // CAS instance.
  std::shared_ptr<llvm::cas::ObjectStore> CAS;
  std::shared_ptr<llvm::cas::ActionCache> ActionCache;
  // Restrict access to the parent scanner class.
  friend class ModuleDependencyScanner;
};

class ModuleDependencyScanner {
public:
  ModuleDependencyScanner(SwiftDependencyScanningService &ScanningService,
                          const CompilerInvocation &ScanCompilerInvocation,
                          const SILOptions &SILOptions,
                          ASTContext &ScanASTContext,
                          DependencyTracker &DependencyTracker,
                          DiagnosticEngine &diags, bool ParallelScan);

  /// Identify the scanner invocation's main module's dependencies
  llvm::ErrorOr<ModuleDependencyInfo>
  getMainModuleDependencyInfo(ModuleDecl *mainModule);

  /// Resolve module dependencies of the given module, computing a full
  /// transitive closure dependency graph.
  std::vector<ModuleDependencyID>
  performDependencyScan(ModuleDependencyID rootModuleID,
                        ModuleDependenciesCache &cache);

  /// Query the module dependency info for the Clang module with the given name.
  /// Explicit by-name lookups are useful for batch mode scanning.
  std::optional<const ModuleDependencyInfo *>
  getNamedClangModuleDependencyInfo(StringRef moduleName,
                                    ModuleDependenciesCache &cache,
                                    ModuleDependencyIDSetVector &discoveredClangModules);

  /// Query the module dependency info for the Swift module with the given name.
  /// Explicit by-name lookups are useful for batch mode scanning.
  std::optional<const ModuleDependencyInfo *>
  getNamedSwiftModuleDependencyInfo(StringRef moduleName,
                                    ModuleDependenciesCache &cache);

  /// How many filesystem lookups were performed by the scanner
  unsigned getNumLookups() { return NumLookups; }

private:
  /// Main routine that computes imported module dependency transitive
  /// closure for the given module.
  /// 1. Swift modules imported directly or via another Swift dependency
  /// 2. Clang modules imported directly or via a Swift dependency
  /// 3. Clang modules imported via textual header inputs to Swift modules
  /// (bridging headers)
  /// 4. Swift overlay modules of all of the transitively imported Clang modules
  /// that have one
  ModuleDependencyIDSetVector
  resolveImportedModuleDependencies(const ModuleDependencyID &rootModuleID,
                                    ModuleDependenciesCache &cache);
  void resolveSwiftModuleDependencies(
      const ModuleDependencyID &rootModuleID, ModuleDependenciesCache &cache,
      ModuleDependencyIDSetVector &discoveredSwiftModules);
  void resolveAllClangModuleDependencies(
      ArrayRef<ModuleDependencyID> swiftModules, ModuleDependenciesCache &cache,
      ModuleDependencyIDSetVector &discoveredClangModules);
  void resolveHeaderDependencies(
      ArrayRef<ModuleDependencyID> swiftModules, ModuleDependenciesCache &cache,
      ModuleDependencyIDSetVector &discoveredHeaderDependencyClangModules);
  void resolveSwiftOverlayDependencies(
      ArrayRef<ModuleDependencyID> swiftModules, ModuleDependenciesCache &cache,
      ModuleDependencyIDSetVector &discoveredDependencies);

  /// Resolve all of a given module's imports to a Swift module, if one exists.
  void resolveSwiftImportsForModule(
      const ModuleDependencyID &moduleID, ModuleDependenciesCache &cache,
      ModuleDependencyIDSetVector &importedSwiftDependencies);

  /// If a module has a bridging header or other header inputs, execute a
  /// dependency scan on it and record the dependencies.
  void resolveHeaderDependenciesForModule(
      const ModuleDependencyID &moduleID, ModuleDependenciesCache &cache,
      ModuleDependencyIDSetVector &headerClangModuleDependencies);

  /// Resolve all module dependencies comprised of Swift overlays
  /// of this module's Clang module dependencies.
  void resolveSwiftOverlayDependenciesForModule(
      const ModuleDependencyID &moduleID, ModuleDependenciesCache &cache,
      ModuleDependencyIDSetVector &swiftOverlayDependencies);

  /// Identify all cross-import overlay module dependencies of the
  /// source module under scan and apply an action for each.
  void resolveCrossImportOverlayDependencies(
      StringRef mainModuleName, ModuleDependenciesCache &cache,
      llvm::function_ref<void(ModuleDependencyID)> action);

  /// Performance BridgingHeader Chaining.
  llvm::Error
  performBridgingHeaderChaining(const ModuleDependencyID &rootModuleID,
                                ModuleDependenciesCache &cache,
                                ModuleDependencyIDSetVector &allModules);

  /// Perform an operation utilizing one of the Scanning workers
  /// available to this scanner.
  template <typename Function, typename... Args>
  auto withDependencyScanningWorker(Function &&F, Args &&...ArgList);

  Identifier getModuleImportIdentifier(StringRef moduleName);

private:
  const CompilerInvocation &ScanCompilerInvocation;
  ASTContext &ScanASTContext;
  DiagnosticEngine &Diagnostics;

  /// The available pool of workers for filesystem module search
  unsigned NumThreads;
  std::list<std::unique_ptr<ModuleDependencyScanningWorker>> Workers;
  llvm::DefaultThreadPool ScanningThreadPool;
  /// Protect worker access.
  std::mutex WorkersLock;
  /// Count of filesystem queries performed
  std::atomic<unsigned> NumLookups = 0;
};

} // namespace language
