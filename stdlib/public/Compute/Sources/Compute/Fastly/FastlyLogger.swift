//
//  Logger.swift
//
//
//  Created by Andrew Barba on 1/12/22.
//

import ComputeRuntime

extension Fastly {
    public struct Logger: Sendable {

        internal let handle: WasiHandle

        public init(name: String) throws {
            var handle: WasiHandle = 0
            try wasi(fastly_log__endpoint_get(name, name.utf8.count, &handle))
            self.handle = handle
        }

        @discardableResult
        public func write(_ messages: String...) throws -> Int {
            let message = messages.joined(separator: " ")
            var result = 0
            try wasi(fastly_log__write(handle, message, message.utf8.count, &result))
            return result
        }

        @discardableResult
        public func write<T>(
            _ value: T,
            encoder: JSONEncoder = .init(),
            formatting: JSONEncoder.OutputFormatting = [.sortedKeys]
        ) throws -> Int where T: Encodable {
            encoder.outputFormatting = formatting
            let data = try encoder.encode(value)
            return try write(data)
        }

        @discardableResult
        public func write(
            _ jsonObject: [String: Any],
            options: JSONSerialization.WritingOptions = [.sortedKeys]
        ) throws -> Int {
            let data = try JSONSerialization.data(withJSONObject: jsonObject, options: options)
            return try write(data)
        }

        @discardableResult
        public func write(
            _ jsonArray: [Any],
            options: JSONSerialization.WritingOptions = [.sortedKeys]
        ) throws -> Int {
            let data = try JSONSerialization.data(withJSONObject: jsonArray, options: options)
            return try write(data)
        }

        @discardableResult
        public func write(_ data: Data) throws -> Int {
            let text = String(bytes: data, encoding: .utf8) ?? ""
            return try write(text)
        }
    }
}
