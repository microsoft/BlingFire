Dir.chdir(__dir__)

pos_dict = String.new("")
tagset = String.new("")

lines = File.read("vocab.bpe").split("\n")[1..-1]
lines.each_with_index do |line, i|
  score = i == 0 ? "-0.00001" : "-#{i}"
  token = line.sub("Ġ", "▁")
  pos_dict << "#{token}\tWORD_ID_#{i + 1}\t#{score}\n"
  tagset << "WORD_ID_#{i + 1} #{i + 1}\n"
end

File.write("pos.dict.utf8", pos_dict)
File.write("tagset.txt", tagset)

system "zip pos.dict.utf8.zip pos.dict.utf8"
