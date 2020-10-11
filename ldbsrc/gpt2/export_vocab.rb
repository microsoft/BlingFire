Dir.chdir(__dir__)

contents = <<~EOS
<pad>\t0
<s>\t0
</s>\t0
<unk>\t0
<sep>\t0
EOS

lines = File.read("vocab.bpe").split("\n")[1..-1]
lines.each_with_index do |line, i|
  token = line.sub("Ġ", "▁")
  contents << "#{token}\t-#{i}\n"
end

File.write("spiece.model.exportvocab.txt", contents)
