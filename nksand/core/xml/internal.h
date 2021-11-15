#pragma once
#include <memory>

namespace qsb { namespace xml { namespace impl_layer {
// -----------------------------------------------------------------------------
//	forward decls.
// -----------------------------------------------------------------------------
	class xml_doc_impl;
	class xml_doc_internal;
	class xml_node_impl;
	class xml_node_internal;

// -----------------------------------------------------------------------------
//  xml_doc_internal
// -----------------------------------------------------------------------------
	class xml_doc_internal {

	}; // class xml_doc_internal

// -----------------------------------------------------------------------------
//  xml_node_internal
// -----------------------------------------------------------------------------
	class xml_node_internal {
	private:
		using this_type = xml_node_internal;

	public:
	// -------------------------------------------------------------------------
	//  constructors
	//
		xml_node_internal() = default;
		xml_node_internal(const this_type&);
		xml_node_internal(this_type&&) noexcept = default;		

	// -------------------------------------------------------------------------
	//  assignments
	//

	// -------------------------------------------------------------------------
	//  get value
	//

	// -------------------------------------------------------------------------
	//  get node
	//

	// -------------------------------------------------------------------------
	//  node update
	//

	private:
		std::unique_ptr<xml_node_impl> pimpl_ = nullptr;

	}; // class xml_node_internal

}}} // namespace qsb::xml::impl_layer
