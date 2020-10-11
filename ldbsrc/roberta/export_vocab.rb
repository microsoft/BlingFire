require "json"

Dir.chdir(__dir__)

contents = ""

lines = JSON.parse(File.read("roberta-base-vocab.json"))
lines.each do |k, v|
  token = k.sub("Ġ", "▁")
  contents << "#{token}\t-#{v}\n"
end

File.write("spiece.model.exportvocab.txt", contents)
