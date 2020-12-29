#pragma once
#include "net-arp.h"
#include "net-dhcp.h"
#include "net-ethernet.h"
#include "net-ipv4.h"
#include "net-utils.h"

#include "options.h"

namespace Net
{
	void Initialize(Options& options);
	void Update();
} // namespace Net
