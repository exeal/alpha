/**
 * @file partition.hpp
 * @author exeal
 * @date 2011-01-21 separated from document.hpp
 */

#ifndef ASCENSION_PARTITION_HPP
#define ASCENSION_PARTITION_HPP
#include <ascension/corelib/basic-types.hpp>
#include <ascension/kernel/content-type.hpp>
#include <ascension/kernel/region.hpp>

namespace ascension {
	namespace kernel {
		class DocumentChange;

		/**
		 * A document partition.
		 * @see DocumentPartitioner#partition
		 */
		struct DocumentPartition {
			ContentType contentType;	///< Content type of the partition.
			Region region;				///< Region of the partition.
			/// Default constructor.
			DocumentPartition() : contentType(ContentType::DEFAULT_CONTENT) BOOST_NOEXCEPT {}
			/// Constructor.
			DocumentPartition(const ContentType& type, const Region& r) BOOST_NOEXCEPT : contentType(type), region(r) {}
		};

		/**
		 * Interface for objects which are interested in getting informed about changes of a document's partitioning.
		 * @see DocumentPartitioner, Document#addPartitioningListener, Document#removePartitioningListener
		 */
		class DocumentPartitioningListener {
		private:
			/**
			 * Document partitions are changed.
			 * @param changedRegion The region whose document partition are changed
			 */
			virtual void documentPartitioningChanged(const Region& changedRegion) = 0;
			friend class Document;
		};

		/**
		 * A document partitioner devides a document into disjoint text partitions.
		 * @see ContentType, Document, DocumentPartition, Document#partitioner,
		 *      Document#setPartitioner, NullPartitioner
		 */
		class DocumentPartitioner {
		public:
			virtual ~DocumentPartitioner() BOOST_NOEXCEPT;
			ContentType contentType(const Position& at) const;
			Document* document() BOOST_NOEXCEPT;
			const Document* document() const BOOST_NOEXCEPT;
			void partition(const Position& at, DocumentPartition& partition) const;
		protected:
			DocumentPartitioner() BOOST_NOEXCEPT;
			void notifyDocument(const Region& changedRegion);
		private:
			/// The document is about to be changed.
			virtual void documentAboutToBeChanged() BOOST_NOEXCEPT = 0;
			/**
			 * The document was changed.
			 * @param change The modification content
			 */
			virtual void documentChanged(const DocumentChange& change) BOOST_NOEXCEPT = 0;
			/**
			 * Returns the partition contains the specified position.
			 * @param at The position. This position is guaranteed to be inside of the document
			 * @param[out] partition The partition
			 */
			virtual void doGetPartition(const Position& at, DocumentPartition& partition) const BOOST_NOEXCEPT = 0;
			/**
			 * Called when the partitioner was connected to a document.
			 * There is not method called @c doUninstall, because a partitioner will be destroyed when disconnected.
			 */
			virtual void doInstall() BOOST_NOEXCEPT = 0;
		private:
			void install(Document& document) BOOST_NOEXCEPT {document_ = &document; doInstall();}
		private:
			Document* document_;
			friend class Document;
		};

		/// @c NullPartitioner always returns one partition covers a whole document.
		class NullPartitioner : public DocumentPartitioner {
		public:
			NullPartitioner() BOOST_NOEXCEPT;
		private:
			void documentAboutToBeChanged() BOOST_NOEXCEPT override;
			void documentChanged(const DocumentChange& change) BOOST_NOEXCEPT override;
			void doGetPartition(const Position& at, DocumentPartition& partition) const BOOST_NOEXCEPT override;
			void doInstall() BOOST_NOEXCEPT override;
		private:
			DocumentPartition p_;
			mutable bool changed_;
		};


		// inline implementation //////////////////////////////////////////////////////////////////
		
		/**
		 * Returns the content type of the partition contains the specified position.
		 * @param at The position
		 * @throw BadPositionException @a position is outside of the document
		 * @throw IllegalStateException The partitioner is not connected to any document
		 * @return The content type
		 */
		inline ContentType DocumentPartitioner::contentType(const Position& at) const {
			DocumentPartition p;
			partition(at, p);
			return p.contentType;
		}
		
		/// Returns the document to which the partitioner connects or @c null.
		inline Document* DocumentPartitioner::document() BOOST_NOEXCEPT {
			return document_;
		}
		
		/// Returns the document to which the partitioner connects or @c null.
		inline const Document* DocumentPartitioner::document() const BOOST_NOEXCEPT {
			return document_;
		}
	}
} // namespace ascension.kernel

#endif // !ASCENSION_PARTITION_HPP
