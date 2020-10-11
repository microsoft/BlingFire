require "json"

Dir.chdir(__dir__)

contents = <<~EOS
<pad>\t0
<s>\t0
</s>\t0
<unk>\t0
<sep>\t0
EOS

lines = JSON.parse(File.read("gpt2-vocab.json"))
lines.each do |k, v|
  token = k.sub("Ġ", "▁")
  contents << "#{token}\t-#{v}\n"
end

File.write("spiece.model.exportvocab.txt", contents)
