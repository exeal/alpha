#!/usr/local/bin/ruby

def buildID(name)
	id = name.gsub(/([[:upper:]])([[:upper:]][[:lower:]])/, '\1_\2')
	id = id.gsub(/([[:lower:]])([[:upper:]])/, '\1_\2')
	id = id.gsub(/-/, '_').upcase()
end

abort('usage: ruby s.rb property-name input-directory') if ARGV.length != 2

property = ARGV[0]
fileName = ARGV[1]
fileName += '/' unless fileName =~ /[\/\\]$/
fileName += 'PropertyValueAliases.txt'
pattern = Regexp.new('^' + property + '\s*;\s([\w\-\/]+)\s+;\s([\w\-]+)(\s+;\s([\w\-]+))?')
input = File.new(fileName)
input.each_line("\n") {|line|
	if m = pattern.match(line)
		id = buildID(m[2])
		print "names_[L\"#{m[1]}\"] = " if m[1] != 'n/a'
		print "names_[L\"#{m[2]}\"] = " if m[2] != m[1]
		print "names_[L\"#{m[4]}\"] = " if m[4]
		print "#{id};\n"
	end
}
