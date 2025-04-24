//
//  StoryboardResource+Generator.swift
//  
//
//  Created by Tom Lokhorst on 2022-06-24.
//

import Foundation
import RswiftResources

extension StoryboardResource {
    public static func generateStruct(storyboards: [StoryboardResource], prefix: SwiftIdentifier) -> Struct {
        let structName = SwiftIdentifier(name: "storyboard")
        let qualifiedName = prefix + structName

        let warning: (String) -> Void = { print("warning: [R.swift]", $0) }

        // Unify different localizations of storyboards
        let unifiedStoryboards = unifyLocalizations(storyboards: storyboards, warning: warning)

        let groupedStoryboards = unifiedStoryboards.grouped(bySwiftIdentifier: { $0.name })
        groupedStoryboards.reportWarningsForDuplicatesAndEmpties(source: "storyboard", result: "file", warning: warning)

        let structs = groupedStoryboards.uniques
            .map { $0.generateStruct(prefix: qualifiedName, warning: warning) }
            .sorted { $0.name < $1.name }

        let comments = ["This `\(qualifiedName.value)` struct is generated, and contains static references to \(structs.count) storyboards."]

        return Struct(comments: comments, name: structName) {
            Init.bundle

            for s in structs {
                s.generateBundleVarGetter(name: s.name.value)
                s.generateBundleFunction(name: s.name.value)
            }

            structs

            if structs.count > 0 {
                generateValidate(structs: structs)
            }
        }
    }

    private static func generateValidate(structs: [Struct]) -> Function {
        let lines = structs
            .map { s -> String in
                let code = "try self.\(s.name.value).validate()"
                return s.deploymentTarget?.codeIf(around: code) ?? code
            }
        return Function(
            comments: [],
            name: .init(name: "validate"),
            params: [],
            returnThrows: true,
            returnType: .void,
            valueCodeString: lines.joined(separator: "\n")
        )
    }

    private static func unifyLocalizations(storyboards: [StoryboardResource], warning: (String) -> Void) -> [StoryboardResource] {
        var result: [StoryboardResource] = []

        for localizations in Dictionary(grouping: storyboards, by: \.name).values {
            guard let storyboard = localizations.first else { continue }
            let ur = storyboard.unify(localizations: localizations)


            let diffs: [String] = [
                ur.differentInitialViewController ? "initial view controllers" : nil,
                ur.differentDeploymentTargets ? "deployment targets" : nil,
            ].compactMap { $0 }

            if diffs.count > 0 {
                warning("Skipping generation of storyboard '\(storyboard.name)', because \(diffs.joined(separator: ", ")) don't match in all localizations")
                continue
            }

            let vcs = ur.differentViewControllerIDs.sorted()
            if vcs.count > 0 {
                let ns = vcs.map { "'\($0)'" }.joined(separator: ", ")
                warning("Skipping generation of \(vcs.count) view controllers in storyboard '\(storyboard.name)', because view controllers \(ns) don't exist (with same class) in all localizations")
            }


            result.append(ur.storyboard)
        }

        return result
    }
}

private extension StoryboardResource {
    func generateStruct(prefix: SwiftIdentifier, warning: (String) -> Void) -> Struct {
        let nameIdentifier = SwiftIdentifier(rawValue: "name")
        let bundleIdentifier = SwiftIdentifier(name: "bundle")
        let reservedIdentifiers: Set<SwiftIdentifier> = [nameIdentifier, bundleIdentifier]

        // View controllers with identifiers
        let grouped = viewControllers
          .compactMap { (vc) -> (identifier: String, vc: StoryboardResource.ViewController)? in
            guard let storyboardIdentifier = vc.storyboardIdentifier else { return nil }
            return (storyboardIdentifier, vc)
          }
          .grouped(bySwiftIdentifier: { $0.identifier })

        grouped.reportWarningsForDuplicatesAndEmpties(source: "view controller", result: "view controller identifier", warning: warning)

        // Warning about conflicts with reserved identifiers
        (grouped.uniques.map(\.identifier) + reservedIdentifiers.map(\.value))
            .grouped(bySwiftIdentifier: { $0 })
            .reportWarningsForReservedNames(source: "view controller", container: "in storyboard '\(name)'", result: "view controller", warning: warning)

        let vargetters = grouped.uniques
            .filter { !reservedIdentifiers.contains(SwiftIdentifier(rawValue: $0.identifier)) }
            .map { (id, vc) in vc.generateVarGetter(identifier: id) }
            .sorted { $0.name < $1.name }

        let letName = LetBinding(
            name: nameIdentifier,
            valueCodeString: "\"\(name)\"")

        let identifier = SwiftIdentifier(name: name)
        let storyboardReference = TypeReference(module: .rswiftResources, rawName: "StoryboardReference")
        let initialContainer = initialViewController == nil ? nil : TypeReference(module: .rswiftResources, rawName: "InitialControllerContainer")

        // additional module reference, for use in validate function
        let additionalModuleReference = isAppKit ? ModuleReference.appKit : ModuleReference.uiKit

        return Struct(
            comments: ["Storyboard `\(name)`."],
            deploymentTarget: deploymentTarget,
            name: identifier,
            protocols: [storyboardReference, initialContainer].compactMap { $0 },
            additionalModuleReferences: [additionalModuleReference]
        ) {
            if let initialViewController = initialViewController {
                TypeAlias(name: "InitialController", value: initialViewController.type)
            }
            Init.bundle

            letName

            vargetters

            generateValidate(viewControllers: grouped.uniques.map(\.vc))
        }
    }

    func generateValidate(viewControllers: [StoryboardResource.ViewController]) -> Function {
        let validateImagesLines = self.usedImageIdentifiers.uniqueAndSorted()
            .map { nameCatalog -> String in
                if isAppKit {
                    if nameCatalog.isSystemCatalog {
                        return "if #available(macOS 11.0, *) { if AppKit.NSImage(systemSymbolName: \"\(nameCatalog.name)\", accessibilityDescription: nil) == nil { throw RswiftResources.ValidationError(\"[R.swift] System image named '\(nameCatalog.name)' is used in storyboard '\(self.name)', but couldn't be loaded.\") } }"
                    } else {
//                        AppKit.NSImage(named:)
                        return "if bundle.image(forResource: \"\(nameCatalog.name)\") == nil { throw RswiftResources.ValidationError(\"[R.swift] Image named '\(nameCatalog.name)' is used in storyboard '\(self.name)', but couldn't be loaded.\") }"
                    }
                } else {
                    if nameCatalog.isSystemCatalog {
                        return "if #available(iOS 13.0, *) { if UIKit.UIImage(systemName: \"\(nameCatalog.name)\") == nil { throw RswiftResources.ValidationError(\"[R.swift] System image named '\(nameCatalog.name)' is used in storyboard '\(self.name)', but couldn't be loaded.\") } }"
                    } else {
                        return "if UIKit.UIImage(named: \"\(nameCatalog.name)\", in: bundle, compatibleWith: nil) == nil { throw RswiftResources.ValidationError(\"[R.swift] Image named '\(nameCatalog.name)' is used in storyboard '\(self.name)', but couldn't be loaded.\") }"
                    }
                }
            }
        let validateColorLines = self.usedColorResources.uniqueAndSorted()
            .filter { !$0.isSystemCatalog }
            .map { nameCatalog in
                isAppKit
                ? "if AppKit.NSColor(named: \"\(nameCatalog.name)\", bundle: bundle) == nil { throw RswiftResources.ValidationError(\"[R.swift] Color named '\(nameCatalog.name)' is used in storyboard '\(self.name)', but couldn't be loaded.\") }"
                : "if UIKit.UIColor(named: \"\(nameCatalog.name)\", in: bundle, compatibleWith: nil) == nil { throw RswiftResources.ValidationError(\"[R.swift] Color named '\(nameCatalog.name)' is used in storyboard '\(self.name)', but couldn't be loaded.\") }"
            }
        let validateViewControllersLines = viewControllers
            .compactMap { vc -> String? in
                guard let storyboardName = vc.storyboardIdentifier else { return nil }
                let storyboardIdentifier = SwiftIdentifier(name: storyboardName)
                return "if \(storyboardIdentifier.value)() == nil { throw RswiftResources.ValidationError(\"[R.swift] ViewController with identifier '\(storyboardIdentifier.value)' could not be loaded from storyboard '\(self.name)' as '\(vc.type.codeString())'.\") }"
            }

        let validateLines = (validateImagesLines + validateColorLines + validateViewControllersLines)

        return Function(
            comments: [],
            name: .init(name: "validate"),
            params: [],
            returnThrows: true,
            returnType: .void,
            valueCodeString: validateLines.joined(separator: "\n")
        )
    }
}

private extension StoryboardResource.ViewController {
    var genericTypeReference: TypeReference {
        TypeReference(
            module: .rswiftResources,
            name: "StoryboardViewControllerIdentifier",
            genericArgs: [self.type]
        )
    }

    func generateVarGetter(identifier: String) -> VarGetter {
        VarGetter(
            name: SwiftIdentifier(name: identifier),
            typeReference: genericTypeReference,
            valueCodeString: #".init(identifier: "\#(identifier.escapedStringLiteral)", storyboard: name, bundle: bundle)"#
        )
    }
}
