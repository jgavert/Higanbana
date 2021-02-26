import json
import os

curDir = os.getcwd() + "\\"

data = "data\\"

file = {}
file["mappings"] = {
  "/data": curDir+data+"data",
  "/screenshot": curDir+data+"screenshot",
  "/shader_binaries": curDir+data+"shader_binaries",
  "/shaders": curDir+data+"shaders",
  "/tmp": curDir+data+"tmp"
}

y = json.dumps(file, indent=2)

f = open("data/mapping.json", "w")
f.write(y)
f.close()