/**
 * @file encoder-factory.hpp
 * @author exeal
 * @date 2004-2014 Was encoder.hpp.
 * @date 2016-09-23 Separated from encoder.hpp.
 */

#ifndef ASCENSION_ENCODER_FACTORY_HPP
#define ASCENSION_ENCODER_FACTORY_HPP
#include <ascension/corelib/basic-types.hpp>
#include <ascension/corelib/string-piece.hpp>
#include <ascension/corelib/encoding/mib-enum.hpp>
#include <boost/core/ignore_unused.hpp>
#include <boost/core/noncopyable.hpp>
#include <locale>
#include <memory>
#include <vector>

namespace ascension {
	namespace encoding {
		/**
		 * An interface which describes the properties of a encoding. @c Encoder#properties method returns this object.
		 * @see EncoderFactory
		 */
		class EncodingProperties {
		public:
			/**
			 * Returns the aliases of the encoding. Default implementation returns an empty.
			 * @return A string contains aliases separated by vertical bar ('|')
			 */
			virtual std::string aliases() const BOOST_NOEXCEPT {return "";}
			/**
			 * Returns the human-readable name of the encoding. Default implementation calls @c #name method.
			 * @param locale The locale used to localize the name
			 * @see #name
			 */
			virtual std::string displayName(const std::locale& locale) const BOOST_NOEXCEPT {
				boost::ignore_unused(locale);
				return name();
			}
			/// Returns the number of bytes represents a UCS character.
			virtual std::size_t maximumNativeBytes() const BOOST_NOEXCEPT = 0;
			/// Returns the number of UCS characters represents a native character. Default implementation returns 1.
			virtual std::size_t maximumUCSLength() const BOOST_NOEXCEPT {return 1;}
			/// Returns the MIBenum value of the encoding.
			virtual MIBenum mibEnum() const BOOST_NOEXCEPT = 0;
			/**
			 * Returns the name of the encoding. If the encoding is registered as a character set in
			 * <a href="http://www.iana.org/assignments/character-sets">IANA character-sets encoding file</a>, should
			 * return the preferred mime name.
			 * @see #displayName
			 */
			virtual std::string name() const BOOST_NOEXCEPT = 0;
			/**
			 * Returns an native character which indicates that the given Unicode character can't map. If
			 * @c Encoder#substitutionPolicy returns @c REPLACE_UNMAPPABLE_CHARACTER, the encoder should use this
			 * character. Default implementation returns 0x1A.
			 */
			virtual Byte substitutionCharacter() const BOOST_NOEXCEPT {return 0x1a;}
		};

		class Encoder;
		class EncoderRegistry;

		/// A factory class creates @c Encoder instances.
		class EncoderFactory : public EncodingProperties {
		public:
			/// Destructor.
			virtual ~EncoderFactory() BOOST_NOEXCEPT {}
		protected:
			/// Returns the @c Encoder instance.
			virtual std::unique_ptr<Encoder> create() const BOOST_NOEXCEPT = 0;
			friend class EncoderRegistry;
		};

		/// Registry of @c EncoderFactory.
		class EncoderRegistry : private boost::noncopyable {
		public:
			static const char ALIASES_SEPARATOR;
		public:
			template<typename OutputIterator> static void availableEncodings(OutputIterator out);
			static EncoderRegistry& instance() BOOST_NOEXCEPT;
			std::unique_ptr<Encoder> forCCSID(int ccsid) BOOST_NOEXCEPT;
			std::unique_ptr<Encoder> forCPGID(int cpgid) BOOST_NOEXCEPT;
			std::unique_ptr<Encoder> forID(std::size_t id) BOOST_NOEXCEPT;
			std::unique_ptr<Encoder> forMIB(MIBenum mib) BOOST_NOEXCEPT;
			std::unique_ptr<Encoder> forName(const boost::string_ref& name) BOOST_NOEXCEPT;
			std::unique_ptr<Encoder> forWindowsCodePage(unsigned int codePage) BOOST_NOEXCEPT;
			void registerFactory(std::shared_ptr<const EncoderFactory> newFactory);
			bool supports(MIBenum mib) BOOST_NOEXCEPT;
			bool supports(const boost::string_ref& name) BOOST_NOEXCEPT;

		private:
			std::shared_ptr<const EncoderFactory> find(MIBenum mib) BOOST_NOEXCEPT;
			std::shared_ptr<const EncoderFactory> find(const boost::string_ref& name) BOOST_NOEXCEPT;
			std::vector<std::shared_ptr<const EncoderFactory>> registry_;
		};

		/**
		 * Returns informations for all available encodings.
		 * @tparam OutputIterator The type of @a out
		 * @param[out] out The output iterator to receive pairs consist of the enumeration identifier and the encoding
		 *             information. The expected type of the pair is
		 *             `std#pair&lt;std::size_t, const EncodingProperties*&gt;`. The enumeration identifier can be used
		 *             with @c #forID method
		 */
		template<typename OutputIterator>
		inline void EncoderRegistry::availableEncodings(OutputIterator out) {
			for(std::size_t i = 0, c = registry().size(); i < c; ++i, ++out)
				*out = std::make_pair(i, registry()[i]);
		}
	}
} // namespace ascension.encoding

#endif // !ASCENSION_ENCODER_FACTORY_HPP
