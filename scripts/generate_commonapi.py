#!/usr/bin/env python3
import re
import sys
from pathlib import Path

if len(sys.argv) != 3:
    raise SystemExit("Usage: generate_commonapi.py <fidl_dir> <output_dir>")

fidl_dir = Path(sys.argv[1]).resolve()
out_dir = Path(sys.argv[2]).resolve()
out_dir.mkdir(parents=True, exist_ok=True)

for fidl_file in sorted(fidl_dir.glob("*.fidl")):
    text = fidl_file.read_text(encoding="utf-8")
    package_match = re.search(r"^package\s+([A-Za-z0-9_\.]+)", text, re.MULTILINE)
    if not package_match:
        continue

    package = package_match.group(1)
    interfaces = re.finditer(r"interface\s+([A-Za-z0-9_]+)\s*\{", text)

    for iface_match in interfaces:
        interface_name = iface_match.group(1)
        iface_block = text[iface_match.start():]
        method_names = re.findall(r"\bmethod\s+([A-Za-z0-9_]+)", iface_block)
        broadcast_names = re.findall(r"\bbroadcast\s+([A-Za-z0-9_]+)", iface_block)

        package_dir = out_dir / package
        package_dir.mkdir(parents=True, exist_ok=True)

        interface_header = package_dir / f"{interface_name}.hpp"
        interface_source = package_dir / f"{interface_name}.cpp"
        proxy_header = package_dir / f"{interface_name}Proxy.hpp"
        proxy_source = package_dir / f"{interface_name}Proxy.cpp"
        stub_header = package_dir / f"{interface_name}Stub.hpp"
        stub_source = package_dir / f"{interface_name}Stub.cpp"

        interface_header_content = f'''#pragma once

#include <string>

namespace {package} {{

class {interface_name} {{
 public:
  virtual ~{interface_name}() = default;
'''
        for method_name in method_names:
            interface_header_content += f"  virtual bool {method_name}(const std::string& payload) = 0;\n"
        for broadcast_name in broadcast_names:
            interface_header_content += f"  virtual void {broadcast_name}(const std::string& payload) = 0;\n"

        interface_header_content += "};\n\n}  // namespace " + package + "\n"
        interface_header.write_text(interface_header_content, encoding="utf-8")

        interface_source_content = f'''#include "{package}/{interface_name}.hpp"

namespace {package} {{

}}  // namespace {package}
'''
        interface_source.write_text(interface_source_content, encoding="utf-8")

        proxy_header_content = f'''#pragma once

#include <string>
#include <utility>

#include "{package}/{interface_name}.hpp"

namespace {package} {{

class {interface_name}Proxy {{
 public:
  explicit {interface_name}Proxy(std::string endpoint = "local") : endpoint_(std::move(endpoint)) {{}}

'''
        proxy_header_content += f"  bool invoke(const std::string& method_name, const std::string& payload) const;\n"
        for method_name in method_names:
            proxy_header_content += f"  bool {method_name}(const std::string& payload) const;\n"
        for broadcast_name in broadcast_names:
            proxy_header_content += f"  void {broadcast_name}(const std::string& payload) const;\n"
        proxy_header_content += "\n private:\n  std::string endpoint_;\n};\n\n}  // namespace " + package + "\n"
        proxy_header.write_text(proxy_header_content, encoding="utf-8")

        proxy_source_content = f'''#include "{package}/{interface_name}Proxy.hpp"

#include <utility>

namespace {package} {{

bool {interface_name}Proxy::invoke(const std::string& method_name, const std::string& payload) const {{
  return !method_name.empty() && !payload.empty();
}}
'''
        for method_name in method_names:
            proxy_source_content += f"bool {interface_name}Proxy::{method_name}(const std::string& payload) const {{\n  return invoke(\"{method_name}\", payload);\n}}\n"
        for broadcast_name in broadcast_names:
            proxy_source_content += f"void {interface_name}Proxy::{broadcast_name}(const std::string& payload) const {{\n  (void)payload;\n}}\n"
        proxy_source_content += "\n}  // namespace " + package + "\n"
        proxy_source.write_text(proxy_source_content, encoding="utf-8")

        stub_header_content = f'''#pragma once

#include <string>

#include "{package}/{interface_name}.hpp"

namespace {package} {{

class {interface_name}Stub : public {interface_name} {{
 public:
  bool getLastMethod() const {{ return last_method_called_; }}\n\n'''
        stub_header_content += "  bool getLastMethod() const;\n"
        for method_name in method_names:
            stub_header_content += f"  bool {method_name}(const std::string& payload) override;\n"
        for broadcast_name in broadcast_names:
            stub_header_content += f"  void {broadcast_name}(const std::string& payload) override;\n"
        stub_header_content += "\n private:\n  bool last_method_called_{false};\n};\n\n}  // namespace " + package + "\n"
        stub_header.write_text(stub_header_content, encoding="utf-8")

        stub_source_content = f'''#include "{package}/{interface_name}Stub.hpp"

namespace {package} {{

bool {interface_name}Stub::getLastMethod() const {{
  return last_method_called_;\n}}
'''
        for method_name in method_names:
            stub_source_content += f"bool {interface_name}Stub::{method_name}(const std::string& payload) {{\n  last_method_called_ = true;\n  return !payload.empty();\n}}\n"
        for broadcast_name in broadcast_names:
            stub_source_content += f"void {interface_name}Stub::{broadcast_name}(const std::string& payload) {{\n  (void)payload;\n}}\n"
        stub_source_content += "\n}  // namespace " + package + "\n"
        stub_source.write_text(stub_source_content, encoding="utf-8")
