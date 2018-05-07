#include <stdlib.h>
#include <stdio.h>

#include "dbus.h"
#include "mako.h"
#include "notification.h"

struct mako_notification *create_notification(struct mako_state *state) {
	struct mako_notification *notif =
		calloc(1, sizeof(struct mako_notification));
	if (notif == NULL) {
		fprintf(stderr, "allocation failed\n");
		return NULL;
	}
	notif->state = state;
	++state->last_id;
	notif->id = state->last_id;
	wl_list_init(&notif->actions);
	notif->urgency = MAKO_NOTIFICATION_URGENCY_UNKNWON;
	wl_list_insert(&state->notifications, &notif->link);
	return notif;
}

void destroy_notification(struct mako_notification *notif) {
	wl_list_remove(&notif->link);
	struct mako_action *action, *tmp;
	wl_list_for_each_safe(action, tmp, &notif->actions, link) {
		wl_list_remove(&action->link);
		free(action->id);
		free(action->title);
		free(action);
	}
	free(notif->app_name);
	free(notif->app_icon);
	free(notif->summary);
	free(notif->body);
	free(notif);
}

void close_notification(struct mako_notification *notif,
		enum mako_notification_close_reason reason) {
	notify_notification_closed(notif, reason);
	destroy_notification(notif);
}

struct mako_notification *get_notification(struct mako_state *state,
		uint32_t id) {
	struct mako_notification *notif;
	wl_list_for_each(notif, &state->notifications, link) {
		if (notif->id == id) {
			return notif;
		}
	}
	return NULL;
}

static bool append_string(char **dst, size_t *dst_cap, const char *src,
		size_t src_len) {
	size_t dst_len = 0;
	if (*dst != NULL) {
		dst_len = strlen(*dst);
	}

	size_t required_cap = dst_len + src_len + 1;
	if (*dst_cap < required_cap) {
		*dst_cap *= 2;
		if (*dst_cap < required_cap) {
			*dst_cap = required_cap;
		}
		if (*dst_cap < 128) {
			*dst_cap = 128;
		}

		*dst = realloc(*dst, *dst_cap);
		if (*dst == NULL) {
			*dst_cap = 0;
			fprintf(stderr, "allocation failed\n");
			return false;
		}
	}

	memcpy(*dst + dst_len, src, src_len);
	(*dst)[dst_len + src_len] = '\0';
	return true;
}

char *format_notification(struct mako_notification *notif, const char *format) {
	char *formatted = NULL;
	size_t formatted_cap = 0;

	const char *last = format;
	while (1) {
		char *current = strchr(last, '%');
		if (current == NULL || current[1] == '\0') {
			append_string(&formatted, &formatted_cap, last, strlen(last));
			break;
		}
		append_string(&formatted, &formatted_cap, last, current - last);

		const char *value = NULL;
		switch (current[1]) {
		case '%':
			value = "%";
			break;
		case 's':
			value = notif->summary;
			break;
		case 'b':
			value = notif->body;
			break;
		}
		if (value == NULL) {
			value = "";
		}

		append_string(&formatted, &formatted_cap, value, strlen(value));
		last = current + 2;
	}

	return formatted;
}
