/**
 * @file encoder-factory.cpp
 * @author exeal
 * @date 2004-2014 Was encoder.hpp.
 * @date 2016-09-23 Separated from encoder.cpp.
 */

#include <ascension/corelib/encoding/encoder.hpp>
#include <ascension/corelib/encoding/encoder-factory.hpp>
#include <ascension/corelib/encoding/encoder-implementation.hpp>
#include <boost/foreach.hpp>


namespace ascension {
	namespace encoding {
		/**
		 * Converts the given encoding name from Unicode into 7-bit US-ASCII can pass to other functions.
		 * @return The converted encoding name
		 * @throw std#bad_alloc Out of memory
		 * @throw UnsupportedEncodingException @a source can't convert
		 */
		std::string encodingNameFromUnicode(const StringPiece& source) {
			const std::unique_ptr<Encoder> encoder(EncoderRegistry::instance().forMIB(fundamental::US_ASCII));
			const std::string temp(encoder->fromUnicode(source));
			if(temp.empty())
				throw UnsupportedEncodingException("invalid encoding name character");
			return temp;
		}

		/// Character separates the string returns @c aliases.
		const char EncoderRegistry::ALIASES_SEPARATOR = '|';

		std::shared_ptr<const EncoderFactory> EncoderRegistry::find(MIBenum mib) BOOST_NOEXCEPT {
			if(mib > MIB_UNKNOWN) {
				BOOST_FOREACH(std::shared_ptr<const EncoderFactory> factory, registry_) {
					if(factory->mibEnum() == mib)
						return factory;
				}
			}
			return std::shared_ptr<const EncoderFactory>();
		}

		std::shared_ptr<const EncoderFactory> EncoderRegistry::find(const boost::string_ref& name) BOOST_NOEXCEPT {
			BOOST_FOREACH(std::shared_ptr<const EncoderFactory> factory, registry_) {
				// test canonical name
				const std::string canonicalName(factory->name());
				if(compareEncodingNames(std::begin(name), std::end(name), std::begin(canonicalName), std::end(canonicalName)) == 0)
					return factory;
				// test aliases
				const std::string aliases(factory->aliases());
				for(std::size_t i = 0; ; ++i) {
					std::size_t delimiter = aliases.find(ALIASES_SEPARATOR, i);
					if(delimiter == std::string::npos)
						delimiter = aliases.length();
					if(delimiter != i) {
						if(compareEncodingNames(std::begin(name), std::end(name), std::begin(aliases) + i, std::begin(aliases) + delimiter) == 0)
							return factory;
						else if(delimiter < aliases.length())
							++delimiter;
					}
					if(delimiter == aliases.length())
						break;
					i = delimiter;
				}
			}
			return std::shared_ptr<const EncoderFactory>();
		}

		/**
		 * Returns the encoder which has the given enumeration identifier.
		 * @param id The identifier obtained by @c availableNames method
		 * @return The encoder or @c null if not registered
		 * @see availableNames
		 */
		std::unique_ptr<Encoder> EncoderRegistry::forID(std::size_t id) BOOST_NOEXCEPT {
			return (id < registry_.size()) ? registry_[id]->create() : std::unique_ptr<Encoder>();
		}

		/**
		 * Returns the encoder which has the given MIBenum value.
		 * @param mib The MIBenum value
		 * @return The encoder or @c null if not registered
		 */
		std::unique_ptr<Encoder> EncoderRegistry::forMIB(MIBenum mib) BOOST_NOEXCEPT {
			const std::shared_ptr<const EncoderFactory> factory(find(mib));
			return (factory.get() != nullptr) ? factory->create() : std::unique_ptr<Encoder>();
		}

		/**
		 * Returns the encoder which matches the given name.
		 * @param name The name
		 * @return The encoder or @c null if not registered
		 */
		std::unique_ptr<Encoder> EncoderRegistry::forName(const boost::string_ref& name) BOOST_NOEXCEPT {
			const std::shared_ptr<const EncoderFactory> factory(find(name));
			return (factory.get() != nullptr) ? factory->create() : std::unique_ptr<Encoder>();
		}

#if BOOST_OS_WINDOWS
		/**
		 * Returns the encoder which has the given Win32 code page.
		 * @param codePage The code page
		 * @return The encoder or @c null if not registered
		 */
		std::unique_ptr<Encoder> EncoderRegistry::forWindowsCodePage(unsigned int codePage) BOOST_NOEXCEPT {
			// TODO: not implemented.
			boost::ignore_unused(codePage);
			return std::unique_ptr<Encoder>();
		}
#endif // BOOST_OS_WINDOWS

		/// Returns the single instance.
		EncoderRegistry& EncoderRegistry::instance() {
			static EncoderRegistry singleton;
			return singleton;
		}

		/**
		 * Registers the new encoder factory.
		 * @param newFactory The encoder factory
		 */
		void EncoderRegistry::registerFactory(std::shared_ptr<const EncoderFactory> newFactory) {
			registry_.push_back(newFactory);
		}

		/// Returns @c true if supports the encoding has the given MIBenum value.
		bool EncoderRegistry::supports(MIBenum mib) BOOST_NOEXCEPT {
			return find(mib) != nullptr;
		}

		/// Returns @c true if supports the encoding has to the given name or alias.
		bool EncoderRegistry::supports(const boost::string_ref& name) BOOST_NOEXCEPT {
			return find(name) != nullptr;
		}


		// US-ASCII and ISO-8859-1 ////////////////////////////////////////////////////////////////////////////////////

		namespace {
			class BasicLatinEncoderFactory : public implementation::EncoderFactoryImpl {
			public:
				BasicLatinEncoderFactory(const std::string& name, MIBenum mib, const std::string& displayName,
					const std::string& aliases, std::uint32_t mask) : implementation::EncoderFactoryImpl(name, mib, displayName, 1, 1, aliases), mask_(mask) {}
				virtual ~BasicLatinEncoderFactory() BOOST_NOEXCEPT {}
				std::unique_ptr<Encoder> create() const override BOOST_NOEXCEPT {
					return std::unique_ptr<Encoder>(new InternalEncoder(mask_, *this));
				}
			private:
				class InternalEncoder : public Encoder {
				public:
					InternalEncoder(std::uint32_t mask, const EncodingProperties& properties) BOOST_NOEXCEPT : mask_(mask), props_(properties) {}
				private:
					Result doFromUnicode(State& state,
						const boost::iterator_range<Byte*>& to, Byte*& toNext, const boost::iterator_range<const Char*>& from, const Char*& fromNext) override;
					Result doToUnicode(State& state,
						const boost::iterator_range<Char*>& to, Char*& toNext, const boost::iterator_range<const Byte*>& from, const Byte*& fromNext) override;
					const EncodingProperties& properties() const override BOOST_NOEXCEPT {return props_;}
				private:
					const std::uint32_t mask_;
					const EncodingProperties& props_;
				};
			private:
				const std::uint32_t mask_;
			};

			struct Installer {
				Installer() BOOST_NOEXCEPT {
					EncoderRegistry::instance().registerFactory(std::make_shared<BasicLatinEncoderFactory>(
						"US-ASCII", fundamental::US_ASCII, "",
						"ANSI_X3.4-1968|iso-ir-6|ANSI_X3.4-1986|ISO_646.irv:1991|ASCII|ISO646-US|us|IBM367|cp367"
						"\0csASCII|iso_646.irv:1983|ascii7|646|windows-20127|ibm-367", 0x7f));
					EncoderRegistry::instance().registerFactory(std::make_shared<BasicLatinEncoderFactory>(
						"ISO-8859-1", fundamental::ISO_8859_1, "Western (ISO 8859-1)",
						"iso-ir-100|ISO_8859-1|latin1|l1|IBM819|CP819|csISOLatin1" "\0ibm-819|8859_1|819", 0xff));
				}
			} unused;

			Encoder::Result BasicLatinEncoderFactory::InternalEncoder::doFromUnicode(State& state,
					const boost::iterator_range<Byte*>& to, Byte*& toNext, const boost::iterator_range<const Char*>& from, const Char*& fromNext) {
				toNext = boost::begin(to);
				fromNext = boost::const_begin(from);
				for(; toNext < boost::end(to) && fromNext < boost::const_end(from); ++toNext, ++fromNext) {
					if((*fromNext & ~mask_) != 0) {
						if(substitutionPolicy() == IGNORE_UNMAPPABLE_CHARACTERS)
							--toNext;
						else if(substitutionPolicy() == REPLACE_UNMAPPABLE_CHARACTERS)
							*toNext = properties().substitutionCharacter();
						else
							return UNMAPPABLE_CHARACTER;
					} else
						*toNext = implementation::mask8Bit(*fromNext);
				}
				return (fromNext == boost::const_end(from)) ? COMPLETED : INSUFFICIENT_BUFFER;
			}

			Encoder::Result BasicLatinEncoderFactory::InternalEncoder::doToUnicode(State& state,
					const boost::iterator_range<Char*>& to, Char*& toNext, const boost::iterator_range<const Byte*>& from, const Byte*& fromNext) {
				toNext = boost::begin(to);
				fromNext = boost::const_begin(from);
				for(; toNext < boost::end(to) && fromNext < boost::const_end(from); ++toNext, ++fromNext) {
					if((*fromNext & ~mask_) != 0) {
						if(substitutionPolicy() == IGNORE_UNMAPPABLE_CHARACTERS)
							--toNext;
						else if(substitutionPolicy() == REPLACE_UNMAPPABLE_CHARACTERS)
							*toNext = text::REPLACEMENT_CHARACTER;
						else
							return UNMAPPABLE_CHARACTER;
					} else
						*toNext = *fromNext;
				}
				return (fromNext == boost::const_end(from)) ? COMPLETED : INSUFFICIENT_BUFFER;
			}
		} // namespace @0
	}
}
