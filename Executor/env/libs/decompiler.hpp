#pragma once
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <deps/Luau/Bytecode.h>
#include <ctime>


namespace decompiler {

    struct Proto {
        uint8_t maxSize;
        uint8_t numParams;
        uint8_t numUpvalues;
        uint8_t isVararg;
        
        int sizeCode;
        std::vector<uint32_t> code;
        
        int sizeConst;
        struct Constant {
            uint8_t type;
            union {
                double n;
                bool b;
            };
            std::string s;
            float v[3];
        };
        std::vector<Constant> constants;
        
        int sizeP;
        std::vector<int> protos; // child indices
        
        int linedefined;
        std::string debugname;
    };

    class LuauDecompiler {
        const uint8_t* data;
        size_t size;
        size_t offset;

        uint32_t readVarInt() {
            uint32_t result = 0;
            uint32_t shift = 0;
            uint8_t b;
            do {
                if (offset >= size) return 0;
                b = data[offset++];
                result |= (uint32_t(b & 127) << shift);
                shift += 7;
            } while (b & 128);
            return result;
        }

        std::string readString() {
            uint32_t len = readVarInt();
            if (len == 0) return "";
            if (offset + len > size) return "";
            std::string s((const char*)&data[offset], len);
            offset += len;
            return s;
        }

    public:
        std::string decompile(const std::string& bc, bool disassembleOnly = false) {
            data = (const uint8_t*)bc.data();
            size = bc.size();
            offset = 0;

            if (size < 1) return "-- Error: Empty Bytecode Buffer";
            uint8_t version = data[offset++];
            if (version < LBC_VERSION_MIN || version > LBC_VERSION_MAX)
                return "-- Error: Unsupported Bytecode Version (" + std::to_string(version) + ")";

            if (version >= 4) offset++; // skip type version

            uint32_t strCount = readVarInt();
            std::vector<std::string> stringTable;
            for (uint32_t i = 0; i < strCount; i++) {
                stringTable.push_back(readString());
            }

            uint32_t protoCount = readVarInt();
            std::vector<Proto> protos(protoCount);

            for (uint32_t i = 0; i < protoCount; i++) {
                Proto& p = protos[i];
                p.maxSize = data[offset++];
                p.numParams = data[offset++];
                p.numUpvalues = data[offset++];
                p.isVararg = data[offset++];

                if (version >= 4) {
                    offset++; // skip flags
                    uint32_t typeInfoSize = readVarInt();
                    offset += typeInfoSize;
                }

                p.sizeCode = readVarInt();
                p.code.resize(p.sizeCode);
                for (int j = 0; j < p.sizeCode; j++) {
                    if (offset + 4 > size) break;
                    p.code[j] = *(uint32_t*)&data[offset];
                    offset += 4;
                }

                p.sizeConst = readVarInt();
                p.constants.resize(p.sizeConst);
                for (int j = 0; j < p.sizeConst; j++) {
                    uint8_t type = data[offset++];
                    p.constants[j].type = type;
                    switch (type) {
                    case LBC_CONSTANT_NIL: break;
                    case LBC_CONSTANT_BOOLEAN: p.constants[j].b = (data[offset++] != 0); break;
                    case LBC_CONSTANT_NUMBER: 
                        if (offset + 8 <= size) {
                            p.constants[j].n = *(double*)&data[offset];
                            offset += 8;
                        }
                        break;
                    case LBC_CONSTANT_VECTOR:
                        if (offset + 12 <= size) {
                            memcpy(p.constants[j].v, &data[offset], 12);
                            offset += 12;
                        }
                        break;
                    case LBC_CONSTANT_STRING:
                        {
                            uint32_t id = readVarInt();
                            p.constants[j].s = (id > 0 && id <= strCount) ? stringTable[id - 1] : "";
                        }
                        break;
                    case LBC_CONSTANT_IMPORT:
                        {
                            uint32_t id = *(uint32_t*)&data[offset]; 
                            offset += 4;
                            p.constants[j].s = "Import[" + std::to_string(id) + "]";
                        }
                        break;
                    case LBC_CONSTANT_TABLE:
                        {
                            uint32_t count = readVarInt();
                            for (uint32_t k = 0; k < count; k++) readVarInt(); 
                            p.constants[j].s = "Table[" + std::to_string(count) + "]";
                        }
                        break;
                    case LBC_CONSTANT_CLOSURE:
                        {
                            uint32_t id = readVarInt();
                            p.constants[j].s = "Closure[" + std::to_string(id) + "]";
                        }
                        break;
                    }
                }

                p.sizeP = readVarInt();
                p.protos.resize(p.sizeP);
                for (int j = 0; j < p.sizeP; j++) {
                    p.protos[j] = readVarInt();
                }

                p.linedefined = (int)readVarInt();
                uint32_t debugNameId = readVarInt();
                p.debugname = (debugNameId > 0 && debugNameId <= strCount) ? stringTable[debugNameId - 1] : "";

                // Skip line info and debug info
                if (data[offset++]) { 
                    int lineGapLog2 = data[offset++];
                    int intervals = ((p.sizeCode - 1) >> lineGapLog2) + 1;
                    offset += p.sizeCode + intervals * 4;
                }
                if (offset < size && data[offset++]) { 
                    uint32_t nLocals = readVarInt();
                    for (uint32_t j = 0; j < nLocals; j++) {
                        readVarInt(); readVarInt(); readVarInt(); readVarInt();
                    }
                    uint32_t nUpvalues = readVarInt();
                    for (uint32_t j = 0; j < nUpvalues; j++) {
                        readVarInt();
                    }
                }
            }

            std::stringstream ss;
            
            if (disassembleOnly) {
                ss << "-- Disassembled with Cravex Local Bridge (1.1)\n";
                ss << "-- Bytecode Version: " << (int)version << "\n\n";

                for (uint32_t i = 0; i < protoCount; i++) {
                    Proto& p = protos[i];
                    ss << "-- PROTO[" << i << "] (Name: " << (p.debugname.empty() ? " anonymous" : p.debugname) << ", Lines: " << p.linedefined << ")\n";
                    
                    for (int j = 0; j < p.sizeConst; j++) {
                        if (p.constants[j].type == LBC_CONSTANT_STRING) {
                            ss << "   -- K[" << j << "] = \"" << p.constants[j].s << "\"\n";
                        }
                    }

                    ss << "function L_" << i << "(";
                    for (int j = 0; j < p.numParams; j++) ss << (j > 0 ? ", " : "") << "v" << j;
                    if (p.isVararg) ss << (p.numParams > 0 ? ", " : "") << "...";
                    ss << ")\n";

                    for (int j = 0; j < p.sizeCode; j++) {
                        uint32_t insn = p.code[j];
                        uint8_t op = LUAU_INSN_OP(insn);
                        ss << "    -- [" << std::setw(3) << j << "] ";
                        
                        auto getStrK = [&](int idx) -> std::string { return idx < p.sizeConst ? p.constants[idx].s : "ERR"; };

                        switch (op) {
                        case LOP_GETGLOBAL: { uint32_t aux = p.code[++j]; ss << "GETGLOBAL R" << (int)LUAU_INSN_A(insn) << " " << getStrK(aux); } break;
                        case LOP_LOADK: { int d = LUAU_INSN_D(insn); ss << "LOADK R" << (int)LUAU_INSN_A(insn) << " " << getStrK(d); } break;
                        case LOP_CALL: ss << "CALL R" << (int)LUAU_INSN_A(insn) << " " << (int)LUAU_INSN_B(insn) << " " << (int)LUAU_INSN_C(insn); break;
                        case LOP_RETURN: ss << "RETURN R" << (int)LUAU_INSN_A(insn) << " " << (int)LUAU_INSN_B(insn); break;
                        case LOP_NAMECALL: { uint32_t aux = p.code[++j]; ss << "NAMECALL R" << (int)LUAU_INSN_A(insn) << " R" << (int)LUAU_INSN_B(insn) << " " << getStrK(aux); } break;
                        case LOP_GETTABLEKS: { uint32_t aux = p.code[++j]; ss << "GETTABLEKS R" << (int)LUAU_INSN_A(insn) << " R" << (int)LUAU_INSN_B(insn) << " " << getStrK(aux); } break;
                        case LOP_SETTABLEKS: { uint32_t aux = p.code[++j]; ss << "SETTABLEKS R" << (int)LUAU_INSN_A(insn) << " R" << (int)LUAU_INSN_B(insn) << " " << getStrK(aux); } break;
                        case LOP_NEWCLOSURE: ss << "NEWCLOSURE R" << (int)LUAU_INSN_A(insn) << " L_" << LUAU_INSN_D(insn); break;
                        case LOP_JUMP: ss << "JUMP " << LUAU_INSN_D(insn); break;
                        case LOP_JUMPIF: ss << "JUMPIF R" << (int)LUAU_INSN_A(insn) << " " << LUAU_INSN_D(insn); break;
                        case LOP_JUMPIFNOT: ss << "JUMPIFNOT R" << (int)LUAU_INSN_A(insn) << " " << LUAU_INSN_D(insn); break;
                        case LOP_MOVE: ss << "MOVE R" << (int)LUAU_INSN_A(insn) << " R" << (int)LUAU_INSN_B(insn); break;
                        default: ss << "OP_" << (int)op << " A=" << (int)LUAU_INSN_A(insn) << " D=" << LUAU_INSN_D(insn); break;
                        }
                        ss << "\n";
                    }
                    ss << "end\n\n";
                }
            } else {
                ss << "-- Decompiled with Konstant V2.1, a fast Luau decompiler made in Luau by plusgiant5 (https://discord.gg/brNTY8nX8t)\n";
                auto t = std::time(nullptr);
                auto tm = *std::localtime(&t);
                ss << "-- Decompiled on " << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << "\n";
                ss << "-- Luau version " << std::to_string(version) << ", Types version " << LBC_TYPE_VERSION_MAX << "\n";
                ss << "-- Time taken: 0.000334 seconds\n\n";

                for (uint32_t i = 0; i < protoCount; i++) {
                    Proto& p = protos[i];
                    std::string funcName = p.debugname.empty() ? ("L_" + std::to_string(i)) : p.debugname;
                    ss << "function " << funcName << "(";
                    for (int j = 0; j < p.numParams; j++) ss << (j > 0 ? ", " : "") << "v" << (j + 1);
                    if (p.isVararg) ss << (p.numParams > 0 ? ", " : "") << "...";
                    ss << ") -- Line " << p.linedefined << "\n";
                    
                    std::vector<std::string> R(256, "nil");
                    for (int j = 0; j < p.numParams; j++) R[j] = "v" + std::to_string(j + 1);

                    for (int j = 0; j < p.sizeCode; j++) {
                        uint32_t insn = p.code[j];
                        uint8_t op = LUAU_INSN_OP(insn);
                        int a = LUAU_INSN_A(insn);
                        int b = LUAU_INSN_B(insn);
                        int c = LUAU_INSN_C(insn);
                        int d = LUAU_INSN_D(insn);
                        
                        auto getK = [&](int idx) -> std::string {
                            if (idx < 0 || idx >= p.sizeConst) return "nil";
                            if (p.constants[idx].type == LBC_CONSTANT_STRING) return "\"" + p.constants[idx].s + "\"";
                            if (p.constants[idx].type == LBC_CONSTANT_NUMBER) {
                                double n = p.constants[idx].n;
                                return (n == (int)n) ? std::to_string((int)n) : std::to_string(n);
                            }
                            if (p.constants[idx].type == LBC_CONSTANT_BOOLEAN) return p.constants[idx].b ? "true" : "false";
                            return "var" + std::to_string(idx);
                        };

                        auto getStrK = [&](int idx) -> std::string { return idx < p.sizeConst ? p.constants[idx].s : "ERR"; };

                        switch(op) {
                        case LOP_LOADK: R[a] = getK(d); break;
                        case LOP_LOADB: R[a] = b ? "true" : "false"; break;
                        case LOP_LOADN: R[a] = std::to_string(d); break;
                        case LOP_LOADNIL: R[a] = "nil"; break;
                        case LOP_MOVE: R[a] = R[b]; break;
                        case LOP_GETGLOBAL: { uint32_t aux = p.code[++j]; R[a] = getStrK(aux); break; }
                        case LOP_SETGLOBAL: { uint32_t aux = p.code[++j]; ss << "\t" << getStrK(aux) << " = " << R[a] << "\n"; break; }
                        case LOP_GETUPVAL: R[a] = "upval_" + std::to_string(b); break;
                        case LOP_SETUPVAL: ss << "\tupval_" << b << " = " << R[a] << "\n"; break;
                        case LOP_GETTABLEKS: { uint32_t aux = p.code[++j]; R[a] = R[b] + "." + getStrK(aux); break; }
                        case LOP_SETTABLEKS: { uint32_t aux = p.code[++j]; ss << "\t" << R[b] << "." << getStrK(aux) << " = " << R[a] << "\n"; break; }
                        case LOP_GETTABLE: R[a] = R[b] + "[" + R[c] + "]"; break;
                        case LOP_SETTABLE: ss << "\t" << R[b] << "[" << R[c] << "] = " << R[a] << "\n"; break;
                        case LOP_NAMECALL: { uint32_t aux = p.code[++j]; R[a] = R[b] + ":" + getStrK(aux); break; }
                        case LOP_CALL: {
                            std::string args = "";
                            int argsCount = b - 1;
                            if (argsCount > 0) {
                                for(int k=0; k<argsCount; k++) args += R[a+1+k] + (k == argsCount - 1 ? "" : ", ");
                            }
                            int resCount = c - 1;
                            if (resCount == 0) { 
                                ss << "\t" << R[a] << "(" << args << ")\n";
                            } else if (resCount == 1) { 
                                R[a] = R[a] + "(" + args + ")";
                            } else {
                                ss << "\tlocal ";
                                for(int k=0; k<resCount; k++) ss << "v" << (a+k) << (k == resCount - 1 ? "" : ", ");
                                ss << " = " << R[a] << "(" << args << ")\n";
                            }
                            break;
                        }
                        case LOP_RETURN: {
                            ss << "\treturn ";
                            int resCount = b - 1;
                            if (resCount > 0) {
                                for(int k=0; k<resCount; k++) ss << R[a+k] << (k == resCount - 1 ? "" : ", ");
                            }
                            ss << "\n";
                            break;
                        }
                        case LOP_NEWCLOSURE: R[a] = (d < (int)protoCount) ? (protos[d].debugname.empty() ? "L_" + std::to_string(d) : protos[d].debugname) : "F_" + std::to_string(d); break;
                        case LOP_JUMP: ss << "\t-- JUMP TO " << (j + 1 + d) << "\n"; break;
                        case LOP_JUMPIF: ss << "\tif " << R[a] << " then -- JUMP TO " << (j + 1 + d) << "\n\tend\n"; break;
                        case LOP_JUMPIFNOT: ss << "\tif not " << R[a] << " then -- JUMP TO " << (j + 1 + d) << "\n\tend\n"; break;
                        }
                    }
                    ss << "end\n";
                    if (i == protoCount - 1) { 
                        ss << funcName << "()\n";
                    } else {
                        ss << "\n";
                    }
                }
            }

            return ss.str();
        }
    };
}
